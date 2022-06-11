while true
    if $irq_instances
        $irq_instances.each do |pin|
            pin.__check_handler() if pin
        end
    end
    sleep_ms 10
end