Sub Main()
    print "Starting."
    ndk = CreateObject("roNDK")
    retval = ndk.start("pkg:/webrtcplayer",["--debug"])
    print "Returned - " ; retval
End Sub
