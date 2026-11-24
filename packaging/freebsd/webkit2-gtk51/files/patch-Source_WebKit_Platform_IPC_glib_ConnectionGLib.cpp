--- Source/WebKit/Platform/IPC/glib/ConnectionGLib.cpp.orig
+++ Source/WebKit/Platform/IPC/glib/ConnectionGLib.cpp
@@ -464,8 +464,14 @@
 
     auto* controlMessagePtr = controlMessage.get();
     GUniqueOutPtr<GError> error;
+#if OS(FREEBSD)
+    // FreeBSD requires MSG_EOR to delimit records on a SOCK_SEQPACKET socket.
+    constexpr int sendFlags = MSG_EOR;
+#else
+    constexpr int sendFlags = 0;
+#endif
     auto bytesWritten = g_socket_send_message(m_socket.get(), nullptr, outputVector, outputVectorLength,
-        controlMessagePtr ? &controlMessagePtr : nullptr, controlMessagePtr ? 1 : 0, 0, m_cancellable.get(), &error.outPtr());
+        controlMessagePtr ? &controlMessagePtr : nullptr, controlMessagePtr ? 1 : 0, sendFlags, m_cancellable.get(), &error.outPtr());
     if (controlMessage) {
         // File descriptors are owned by UnixMessage, so steal them from the control message to avoid a double close.
         g_free(g_unix_fd_message_steal_fds(G_UNIX_FD_MESSAGE(controlMessage.get()), nullptr));
