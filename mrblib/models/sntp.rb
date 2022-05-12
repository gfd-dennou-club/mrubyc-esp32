# coding: utf-8

class SNTP

  # 内部関数
  def to_bcd( tt )
    (tt / 10).to_i(2) << 4 | (tt % 10).to_i(2)
  end
    
  # 初期化
  def initialize()
    @time = sntp_init()
  end

  # 時刻取得
  def year()
    @time[0] + 1900
  end

  def year2()
    (@time[0] + 1900) % 2000
  end

  def mon()
    @time[1]
  end

  def mday()
    @time[2]
  end

  def wday()
    @time[3]
  end

  def hour()
    @time[4]
  end

  def min()
    @time[5]
  end

  def sec()
    @time[6]
  end
  
  # 時刻取得 (BCD コード)
  def year_bcd()
    to_bcd( @time[0] + 1900 )
  end

  def year2_bcd()
    to_bcd( (@time[0] + 1900) % 2000 )
  end

  def mon_bcd()
    to_bcd( @time[1] )
  end

  def mday_bcd()
    to_bcd( @time[2] )
  end

  def wday_bcd()
    to_bcd( @time[3] )
  end

  def hour_bcd()
    to_bcd( @time[4] )
  end

  def min_bcd()
    to_bcd( @time[5] )
  end

  def sec_bcd()
    to_bcd( @time[6] )
  end
  
end
