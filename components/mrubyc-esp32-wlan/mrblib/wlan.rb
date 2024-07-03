# coding: utf-8

class WLAN2


  
  ###
  ### HTTP_client
  ###
  def invoke( url )
    httpclient_init( url )
    httpclient_invoke()
    httpclient_cleanup()
  end

  def access( url )
    httpclient_init( url )
    httpclient_invoke()
    httpclient_cleanup()
  end
 
end
