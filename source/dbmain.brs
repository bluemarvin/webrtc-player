Sub Main()
    print "Starting gdbserver webrtcplayer."
    ndk = CreateObject("roNDK")
    retval = ndk.start("pkg:/gdbserver",[":5555", "pkg_/webrtcplayer"])
    print "Returned - " ; retval
End Sub
