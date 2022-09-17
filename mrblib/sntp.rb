# coding: utf-8

class SNTP
    
  # 初期化
  def initialize()
    @time = sntp_init()
  end

  # 数字の配列を戻す. RTC の入力として使うことを想定
  def read()
    return [(@time[0] + 1900) % 2000, @time[1], @time[2], @time[3], @time[4], @time[5], @time[6]]
  end

  # 数字の配列を戻す. RTC の入力として使うことを想定
  def datetime
    read()
  end

  # 文字列で日付を戻す
  def str_date()
    return sprintf("%02d-%02d-%02d", (@time[0] + 1900) % 2000, @time[1], @time[2]).to_s
  end

  # 文字列で時刻を戻す
  def str_time()
    return sprintf("%02d:%02d:%02d", @time[4], @time[5], @time[6]).to_s
  end  

  # 文字列で日時を戻す
  def str_datetime()
    return sprintf("%04d%02d%02d%02d%02d%02d", @time[0] + 1900, @time[1], @time[2], @time[4], @time[5], @time[6]).to_s
  end

  # 時刻取得
  def year()
    return @time[0].to_i + 1900
  end

  def year2()
    return (@time[0].to_i + 1900) % 2000
  end

  def mon()
    return @time[1].to_i
  end

  def mday()
    return @time[2].to_i
  end

  def wday()
    return @time[3].to_i
  end

  def hour()
    return @time[4].to_i
  end

  def min()
    return @time[5].to_i
  end

  def sec()
    return @time[6].to_i
  end

end
