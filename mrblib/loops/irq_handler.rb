while true
    $irq_instances.each do |pin|
        pin.__check_handler() if pin
    end
    sleep_ms 10
end