# coding: utf-8

class SNTP
    
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

end
