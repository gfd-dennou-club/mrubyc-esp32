class UART
    # 定数
    PARITY_DISABLE = 0
    PARITY_EVEN = 2
    PARITY_ODD = 3

    # 初期化
    def initialize(uart_num, baudrate = 9600, bits = 8, parity = nil, stop = 1)
        @uart_num = uart_num
        @gets_mode = 0
        UART.config(uart_num, baudrate, bits - 5, __get_paritycode(parity), stop)
        UART.set_pin(uart_num)
        UART.driver_install(uart_num)
    end

    # コンストラクタ外からの初期化
    def init(baudrate = 9600, bits = 8, parity = nil, stop = 1)
        UART.config(@uart_num, baudrate, bits - 5, __get_paritycode(parity), stop)
        UART.set_pin(@uart_num)
        UART.driver_install(@uart_num)
    end

    # UARTドライバーの削除
    def deinit
        UART.driver_delete(@uart_num)
    end

    # 指定された文字列を出力する
    def write(data)
        UART.write_bytes(@uart_num, data)
    end

    # 指定されたバイト数のデータを読み込む
    # 指定されたバイト数のデータが到着していない場合、nilを返す
    def read(bytes)
        UART.read_bytes(@uart_num, bytes, 0)
    end

    # 指定されたバイト数のデータを読み込む
    # 指定されたバイト数のデータが到着していない場合、到着している分のデータを返す
    def read_nonblock(maxlen)
        UART.read_bytes(@uart_num, maxlen, 1)
    end

    # 文字列を一行読み込む。実際には受信キュー内の "\n" までのバイト列を返す
    # 受信キューに "\n" が無い場合、nilを返す
    def gets()
        UART.read_gets(@uart_num, @gets_mode)
    end

    # getsにおいて、\r単体を改行として判断するかどうかのフラグを設定する
    # trueにすると\r\nを改行として判定しなくなるため、なにか不都合がない限り変更する必要はない
    def set_gets_mode(mode)
        @gets_mode = mode
    end

    # ​読み込みバッファをクリアする
    def clear_tx_buffer
        UART.flush_input(@uart_num)
    end

    # 書き込みバッファをクリアする
    def clear_rx_buffer
        UART.flush(@uart_num)
    end

    def __get_paritycode(parity)
        if parity == nil
            UART::PARITY_DISABLE
        elsif parity == 0
            UART::PARITY_EVEN
        elsif parity == 1
            UART::PARITY_ODD
        end
    end
end