#!/usr/bin/ruby
require 'net/smtp'

if (ARGV.length > 0)
    $message = ARGV[0].to_s
else
    $message = "no message"
end

if (ARGV.length > 1)
    $title = ARGV[1].to_s
else
    $title = "no title"
end



message = <<MESSAGE_END
From: Server <server@nodomain.com>
To: Arpad Rimmel <arpad.rimmel@gmail.com>
Subject: #{$title}

#{$message}
MESSAGE_END

smtp = Net::SMTP.new('smtp.gmail.com',587)
smtp.enable_starttls
smtp.start('gmail.com','mail.sender.2245@gmail.com', 'pass2245', :login)
smtp.send_message message, 'mail.sender@gmail.com', 'arpad.rimmel@gmail.com'
smtp.finish
