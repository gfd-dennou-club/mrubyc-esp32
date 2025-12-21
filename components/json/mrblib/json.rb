class JSON
  # Public API
  def self.parse(str)
    p = Parser.new(str || "")
    v = p.parse
    return nil if p.error?
    v
  end

  def self.generate(obj)
    Generator.new.generate(obj)
  end

  def self.pretty_generate(obj, indent = 2)
    Pretty.new(indent).emit(obj, 0)
  end

  # ---------------- Parser ----------------
  class Parser
    def initialize(str)
      @s = str || ""
      @i = 0
      @n = @s.length
      @err = false
      skip_bom
    end

    def error?; @err; end

    def parse
      skip_ws
      v = parse_value
      return nil if @err
      skip_ws  # 末尾の NUL/空白を無視
      if @i != @n
        @err = true
        return nil
      end
      v
    end

    def parse_value
      skip_ws
      c = peek_char
      return fail if c.nil?
      case c
      when "{"
        parse_object
      when "["
        parse_array
      when "\""
        parse_string
      when "-", "0","1","2","3","4","5","6","7","8","9"
        parse_number
      else
        if match_ahead?("true")
          @i += 4; true
        elsif match_ahead?("false")
          @i += 5; false
        elsif match_ahead?("null")
          @i += 4; nil
        else
          fail
        end
      end
    end

    def parse_object
      return fail unless consume_char("{")
      skip_ws
      h = {}
      if peek_char == "}"
        consume_char("}")
        return h
      end
      loop do
        skip_ws
        k = parse_string
        return fail if @err || k.nil?
        skip_ws
        return fail unless consume_char(":")
        skip_ws
        v = parse_value
        return fail if @err
        h[k] = v
        skip_ws
        if consume_char("}")
          break
        end
        return fail unless consume_char(",")
      end
      h
    end

    def parse_array
      return fail unless consume_char("[")
      skip_ws
      a = []
      if peek_char == "]"
        consume_char("]")
        return a
      end
      loop do
        v = parse_value
        return fail if @err
        a << v
        skip_ws
        if consume_char("]")
          break
        end
        return fail unless consume_char(",")
      end
      a
    end

    def parse_string
      return fail unless consume_char("\"")
      parts = []
      seg_start = @i
      while !eof?
        c = peek_char
        if c == "\""
          # 直前までのセグメントを追加
          if @i > seg_start
            parts << substr(seg_start, @i - seg_start)
          end
          @i += 1  # closing quote
          return parts.join
        elsif c == "\\"
          # セグメント切り出し
          if @i > seg_start
            parts << substr(seg_start, @i - seg_start)
          end
          @i += 1
          esc = peek_char
          return fail if esc.nil?
          @i += 1
          case esc
          when "\"","\\","/"
            parts << esc
          when "b" then parts << "\b"
          when "f" then parts << "\f"
          when "n" then parts << "\n"
          when "r" then parts << "\r"
          when "t" then parts << "\t"
          when "u"
            # \uXXXX はそのままリテラル保存（環境依存を避ける）
            hex = substr(@i, 4)
            return fail if hex.nil? || hex.length < 4
            parts << "\\u" + hex
            @i += 4
          else
            return fail
          end
          seg_start = @i
        else
          @i += 1
        end
      end
      fail
    end

    def parse_number
      start = @i
      # sign
      if peek_char == "-"
        @i += 1
      end
      # int
      if peek_char == "0"
        @i += 1
      else
        return fail unless digit_char?(peek_char)
        while digit_char?(peek_char); @i += 1; end
      end
      # frac
      if peek_char == "."
        @i += 1
        return fail unless digit_char?(peek_char)
        while digit_char?(peek_char); @i += 1; end
      end
      # exp
      c = peek_char
      if c == "e" || c == "E"
        @i += 1
        c2 = peek_char
        if c2 == "+" || c2 == "-"
          @i += 1
        end
        return fail unless digit_char?(peek_char)
        while digit_char?(peek_char); @i += 1; end
      end
      num_str = substr(start, @i - start)
      return fail if num_str.nil? || num_str.length == 0

      # 桁数が大きい整数は文字列のまま返してオーバーフロー回避
      if (num_str.index(".") || num_str.index("e") || num_str.index("E"))
        num_str.to_f
      else
        int_digits = num_str[0,1] == "-" ? (num_str.length - 1) : num_str.length
        if int_digits >= 10   # 閾値は用途に応じて調整可（32bit Fixnum 想定）
          num_str             # 文字列のまま
        else
          num_str.to_i
        end
      end
    end

    # ---- helpers ----
    def skip_bom
      if @n >= 3 && @s[0,1] == "\xEF" && @s[1,1] == "\xBB" && @s[2,1] == "\xBF"
        @i = 3
      end
    end

    def skip_ws
      while !eof?
        c = peek_char
        # 空白 / LF / CR / TAB / NUL をスキップ
        if c == " " || c == "\n" || c == "\r" || c == "\t" || c == "\x00"
          @i += 1
        else
          break
        end
      end
    end

    def consume_char(ch)
      return false if eof?
      if peek_char == ch
        @i += 1
        true
      else
        false
      end
    end

    def peek_char
      return nil if eof?
      substr(@i, 1)
    end

    def substr(pos, len)
      @s[pos, len]   # String#[] 2引数版のみ使用
    end

    def eof?
      @i >= @n
    end

    def digit_char?(c)
      c && c >= "0" && c <= "9"
    end

    def match_ahead?(kw)
      len = kw.length
      return false if (@n - @i) < len
      substr(@i, len) == kw
    end

    def fail
      @err = true
      nil
    end
  end

  # ---------------- Generator ----------------
  class Generator
    def generate(v)
      case v
      when Hash
        parts = []
        v.each do |k, val|
          parts << "#{string(k.to_s)}:#{generate(val)}"
        end
        "{#{parts.join(',')}}"
      when Array
        "[#{v.map { |e| generate(e) }.join(',')}]"
      when String
        string(v)
      when TrueClass
        "true"
      when FalseClass
        "false"
      when NilClass
        "null"
      when Integer, Float
        v.to_s
      else
        string(v.to_s)
      end
    end

    def string(s)
      out = []
      out << "\""
      i = 0
      n = s.length
      while i < n
        c = s[i,1]
        case c
        when "\"" then out << "\\\""
        when "\\" then out << "\\\\"
        when "/"  then out << "\\/"
        when "\b" then out << "\\b"
        when "\f" then out << "\\f"
        when "\n" then out << "\\n"
        when "\r" then out << "\\r"
        when "\t" then out << "\\t"
        else
          # 制御文字の \u00XX 化は省略（必要なら追加可能）
          out << c
        end
        i += 1
      end
      out << "\""
      out.join
    end
  end

  # ---------------- Pretty Printer ----------------
  class Pretty
    def initialize(indent); @indent = indent; end

    def emit(v, level)
      case v
      when Hash
        return "{}" if v.empty?
        parts = []
        v.each do |k, val|
          parts << (" " * ((level + 1) * @indent)) +
                   "#{JSON::Generator.new.string(k.to_s)}: " + emit(val, level + 1)
        end
        "{\n" + parts.join(",\n") + "\n" + (" " * (level * @indent)) + "}"
      when Array
        return "[]" if v.empty?
        parts = v.map { |e| (" " * ((level + 1) * @indent)) + emit(e, level + 1) }
        "[\n" + parts.join(",\n") + "\n" + (" " * (level * @indent)) + "]"
      when String
        JSON::Generator.new.string(v)
      when TrueClass
        "true"
      when FalseClass
        "false"
      when NilClass
        "null"
      when Integer, Float
        v.to_s
      else
        JSON::Generator.new.string(v.to_s)
      end
    end
  end
end
