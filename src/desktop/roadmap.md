```
1. RemoteDesktop.CreateSession(options)
       ↓
2. returns request object path

3. wait for Request::Response signal
       ↓
4. extract session_handle

5. Clipboard.RequestClipboard(session_handle, options)

6. Clipboard.SelectionRead(session_handle, "text/plain")
       ↓
7. receive UNIX_FD
       ↓
8. read(fd)
```