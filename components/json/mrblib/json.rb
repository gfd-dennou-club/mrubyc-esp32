#
# JSON parser and generator for mruby/c
#
# Usage:
#
#   data = JSON.parse(json_string)
#   data = JSON.parse_first(truncated_json_string)
#
#   json_string = JSON.generate(data)
#   pretty_string = JSON.pretty_generate(data)
#
#   puts JSON.pretty_generate(data)
#

class JSON

  #
  # 完全なJSON文字列を解析する
  #
  # JSONの末尾まで正しく閉じている必要がある
  #
  def self.parse(text)
    Parser.new(text).parse
  end

  #
  # ルートオブジェクトの先頭レコードだけを解析する
  #
  # 後続部分が途中で切れていても、
  # 先頭レコードが完全なら解析できる
  #
  # 戻り値の例:
  #
  # {
  #   "20260928120000" => {
  #     "temp" => [23.4, 0]
  #   }
  # }
  #
  def self.parse_first(text)
    Parser.new(text).parse_first_member
  end

  #
  # RubyオブジェクトからJSON文字列を生成する
  #
  def self.generate(value)
    Generator.generate(value)
  end

  #
  # Rubyオブジェクトから整形済みJSON文字列を生成する
  #
  def self.pretty_generate(value)
    Generator.pretty_generate(value)
  end


  #
  # JSONパーサー本体
  #
  class Parser

    def initialize(text)
      if text == nil
        raise "JSON parse error: 入力がnilです"
      end

      @text = text
      @pos = 0
      @length = text.length
    end

    #
    # JSON全体を解析する
    #
    def parse
      skip_spaces

      if @pos >= @length
        error("入力が空です")
      end

      value = parse_value

      skip_spaces

      if @pos != @length
        error("JSONの後ろに余分な文字があります")
      end

      value
    end

    #
    # ルートオブジェクトの先頭要素だけ解析する
    #
    # JSON全体の末尾は確認しない
    #
    def parse_first_member
      skip_spaces

      if @pos >= @length
        error("入力が空です")
      end

      expect("{")
      skip_spaces

      # 空のオブジェクト
      if current_char == "}"
        @pos += 1
        return {}
      end

      unless current_char == '"'
        error("オブジェクトのキーは文字列である必要があります")
      end

      key = parse_string

      skip_spaces
      expect(":")
      skip_spaces

      # 先頭レコードの値を完全に解析する
      value = parse_value

      # 後続のカンマやレコードは解析しない
      result = {}
      result[key] = value

      result
    end

    private

    #
    # JSONの値を解析する
    #
    def parse_value
      skip_spaces

      if @pos >= @length
        error("値の途中で入力が終了しました")
      end

      ch = current_char

      if ch == "{"
        parse_object
      elsif ch == "["
        parse_array
      elsif ch == '"'
        parse_string
      elsif ch == "t"
        parse_true
      elsif ch == "f"
        parse_false
      elsif ch == "n"
        parse_null
      elsif ch == "-" || digit?(ch)
        parse_number
      else
        error("値として解釈できない文字です")
      end
    end

    #
    # ObjectをHashへ変換する
    #
    def parse_object
      result = {}

      expect("{")
      skip_spaces

      if current_char == "}"
        @pos += 1
        return result
      end

      loop do
        skip_spaces

        if @pos >= @length
          error("オブジェクトの途中で入力が終了しました")
        end

        unless current_char == '"'
          error("オブジェクトのキーは文字列である必要があります")
        end

        key = parse_string

        skip_spaces
        expect(":")
        skip_spaces

        result[key] = parse_value

        skip_spaces

        if @pos >= @length
          error("オブジェクトが閉じられていません")
        end

        ch = current_char

        if ch == ","
          @pos += 1
        elsif ch == "}"
          @pos += 1
          break
        else
          error("オブジェクトの区切りが不正です")
        end
      end

      result
    end

    #
    # ArrayをArrayへ変換する
    #
    def parse_array
      result = []

      expect("[")
      skip_spaces

      if current_char == "]"
        @pos += 1
        return result
      end

      loop do
        skip_spaces

        if @pos >= @length
          error("配列の途中で入力が終了しました")
        end

        result << parse_value

        skip_spaces

        if @pos >= @length
          error("配列が閉じられていません")
        end

        ch = current_char

        if ch == ","
          @pos += 1
        elsif ch == "]"
          @pos += 1
          break
        else
          error("配列の区切りが不正です")
        end
      end

      result
    end

    #
    # JSON文字列を解析する
    #
    def parse_string
      expect('"')

      result = ""

      while @pos < @length
        ch = current_char
        @pos += 1

        if ch == '"'
          return result
        end

        if ch == "\\"
          if @pos >= @length
            error("文字列が途中で終了しました")
          end

          escaped = current_char
          @pos += 1

          if escaped == '"'
            result << '"'
          elsif escaped == "\\"
            result << "\\"
          elsif escaped == "/"
            result << "/"
          elsif escaped == "b"
            result << "\b"
          elsif escaped == "f"
            result << "\f"
          elsif escaped == "n"
            result << "\n"
          elsif escaped == "r"
            result << "\r"
          elsif escaped == "t"
            result << "\t"
          elsif escaped == "u"
            # mruby/c環境によるUnicode処理の違いを避けるため、
            # \uXXXXをエスケープ表現のまま保持する
            result << "\\u"
            result << read_hex4
          else
            error("不正なエスケープ文字です")
          end
        else
          result << ch
        end
      end

      error("文字列を閉じる引用符がありません")
    end

    #
    # \uXXXXの4桁を読み取る
    #
    def read_hex4
      if @pos + 4 > @length
        error("Unicodeエスケープが途中で終了しました")
      end

      hex = @text[@pos, 4]

      i = 0

      while i < 4
        ch = hex[i, 1]

        unless hex_digit?(ch)
          error("Unicodeエスケープが不正です")
        end

        i += 1
      end

      @pos += 4

      hex
    end

    #
    # JSON数値を解析する
    #
    def parse_number
      start = @pos
      is_float = false

      if current_char == "-"
        @pos += 1

        if @pos >= @length
          error("数値が途中で終了しました")
        end
      end

      if current_char == "0"
        @pos += 1
      else
        unless digit?(current_char)
          error("数値の整数部分が不正です")
        end

        while digit?(current_char)
          @pos += 1
        end
      end

      # 小数部分
      if current_char == "."
        is_float = true
        @pos += 1

        unless digit?(current_char)
          error("数値の小数部分が不正です")
        end

        while digit?(current_char)
          @pos += 1
        end
      end

      # 指数部分
      if current_char == "e" || current_char == "E"
        is_float = true
        @pos += 1

        if current_char == "+" || current_char == "-"
          @pos += 1
        end

        unless digit?(current_char)
          error("数値の指数部分が不正です")
        end

        while digit?(current_char)
          @pos += 1
        end
      end

      number_text = @text[start, @pos - start]

      if is_float
        number_text.to_f
      else
        number_text.to_i
      end
    end

    def parse_true
      expect_word("true")
      true
    end

    def parse_false
      expect_word("false")
      false
    end

    def parse_null
      expect_word("null")
      nil
    end

    #
    # 指定文字列の一致を確認する
    #
    def expect_word(word)
      actual = @text[@pos, word.length]

      if actual != word
        error("'#{word}'が必要です")
      end

      @pos += word.length
    end

    #
    # 指定文字の一致を確認する
    #
    def expect(expected)
      if @pos >= @length
        error("'#{expected}'の前で入力が終了しました")
      end

      if current_char != expected
        error("'#{expected}'が必要です")
      end

      @pos += 1
    end

    #
    # 空白文字を読み飛ばす
    #
    def skip_spaces
      while @pos < @length
        ch = current_char

        if ch == " " ||
           ch == "\t" ||
           ch == "\r" ||
           ch == "\n"
          @pos += 1
        else
          break
        end
      end
    end

    #
    # 現在位置の1文字を取得する
    #
    def current_char
      return nil if @pos >= @length

      @text[@pos, 1]
    end

    def digit?(ch)
      return false if ch == nil

      ch >= "0" && ch <= "9"
    end

    def hex_digit?(ch)
      return false if ch == nil

      (ch >= "0" && ch <= "9") ||
        (ch >= "a" && ch <= "f") ||
        (ch >= "A" && ch <= "F")
    end

    def error(message)
      raise "JSON parse error at #{@pos}: #{message}"
    end
  end


  #
  # JSONジェネレーター本体
  #
  class Generator

    def self.generate(value)
      encode(value, false, 0)
    end

    def self.pretty_generate(value)
      encode(value, true, 0)
    end

    def self.encode(value, pretty, level)
      if value == nil
        "null"
      elsif value == true
        "true"
      elsif value == false
        "false"
      elsif value.is_a?(String)
        encode_string(value)
      elsif value.is_a?(Integer)
        value.to_s
      elsif value.is_a?(Float)
        encode_float(value)
      elsif value.is_a?(Hash)
        encode_hash(value, pretty, level)
      elsif value.is_a?(Array)
        encode_array(value, pretty, level)
      else
        raise "JSON generate error: 未対応の型です"
      end
    end

    def self.encode_hash(hash, pretty, level)
      return "{}" if hash.length == 0

      parts = []

      hash.each do |key, value|
        unless key.is_a?(String)
          raise "JSON generate error: Hashのキーは文字列である必要があります"
        end

        encoded_key = encode_string(key)
        encoded_value = encode(value, pretty, level + 1)

        if pretty
          parts << indent(level + 1) +
                   encoded_key +
                   ": " +
                   encoded_value
        else
          parts << encoded_key +
                   ":" +
                   encoded_value
        end
      end

      if pretty
        "{\n" +
          parts.join(",\n") +
          "\n" +
          indent(level) +
          "}"
      else
        "{" + parts.join(",") + "}"
      end
    end

    def self.encode_array(array, pretty, level)
      return "[]" if array.length == 0

      parts = []

      array.each do |value|
        encoded_value = encode(value, pretty, level + 1)

        if pretty
          parts << indent(level + 1) +
                   encoded_value
        else
          parts << encoded_value
        end
      end

      if pretty
        "[\n" +
          parts.join(",\n") +
          "\n" +
          indent(level) +
          "]"
      else
        "[" + parts.join(",") + "]"
      end
    end

    def self.encode_string(string)
      result = '"'

      i = 0

      while i < string.length
        ch = string[i, 1]

        if ch == '"'
          result << '\\"'
        elsif ch == "\\"
          result << "\\\\"
        elsif ch == "\b"
          result << "\\b"
        elsif ch == "\f"
          result << "\\f"
        elsif ch == "\n"
          result << "\\n"
        elsif ch == "\r"
          result << "\\r"
        elsif ch == "\t"
          result << "\\t"
        else
          result << ch
        end

        i += 1
      end

      result << '"'

      result
    end

    def self.encode_float(value)
      value.to_s
    end

    def self.indent(level)
      "  " * level
    end
  end
end

=begin
#
# HashからJSONを生成するための補助メソッド
#
class Hash
  def generate
    JSON.generate(self)
  end

  def pretty_generate
    JSON.pretty_generate(self)
  end
end


#
# ArrayからJSONを生成するための補助メソッド
#
class Array
  def generate
    JSON.generate(self)
  end

  def pretty_generate
    JSON.pretty_generate(self)
  end
end
=end
