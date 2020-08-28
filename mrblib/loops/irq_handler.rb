while true
    $irq_instances.each do |pin|
        pin.check_handler() if pin
    end
    sleep 0.01
end