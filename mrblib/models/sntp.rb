# coding: utf-8

class SNTP

  # 初期化
  def initialize()
    @time = sntp_init()
  end

  # 時刻取得
  def year()
    @time[0]
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

  # BCD コード
  def to_bcd( tt )
    (tt / 10).to_i(2) << 4 | (tt % 10).to_i(2)
  end
  
end
