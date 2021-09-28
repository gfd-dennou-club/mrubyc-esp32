display = ILI934X.new(23, 18, 14, 27, 33, 32)
sleep 10
display.drawRectangle(20, 300, 20, 170, ILI934X.color(0x69, 0xba, 0xf5))
display.drawRectangle(20, 300, 170, 180, ILI934X.color(0x28, 0xad, 0x35))
display.drawRectangle(20, 300, 180, 220, ILI934X.color(0x55, 0x42, 0x3d))
x = [200, 230, 170]
y = [150, 200, 200]
3.times do |i|
  drawLine(display, x[i], x[(i + 1) % 3], y[i], y[(i + 1) % 3], ILI934X.color(0x96, 0x56, 0xa1), 5)
end
display.writeString("12345:\n67890.", 50, 50, 20, 35, 30, ILI934X.color(0x27, 0x1c, 0x19), nil)
display.writeString("12345:\n67890.", 45, 45, 20, 35, 30, ILI934X.color(0xff, 0xf3, 0xec), nil)