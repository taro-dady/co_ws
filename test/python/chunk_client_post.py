
import http.client

def chunk_data(data, chunk_size, conn):
    dl = len(data)
    for i in range(dl // chunk_size):
        ret : str = ""
        ret += "%s\r\n" % (hex(chunk_size)[2:])
        ret += "%s\r\n" % (data[i * chunk_size : (i + 1) * chunk_size])
        conn.send(ret.encode())
    
    if len(data) % chunk_size != 0:
        ret = ""
        ret += "%s\r\n" % (hex(len(data) % chunk_size)[2:])
        ret += "%s\r\n" % (data[-(len(data) % chunk_size):])
        conn.send(ret.encode())
    conn.send("0\r\n\r\n".encode())

body = "<html><head><title>taro</title></head><body>hello, response</body></html>"
size_per_chunk = 10
conn = http.client.HTTPConnection('127.0.0.1', 20002)
url = "/post_chunk"
conn.putrequest('POST', url)
conn.putheader('Transfer-Encoding', 'chunked')
conn.endheaders()
# conn.send(chunk_data(body, size_per_chunk).encode('utf-8'))
chunk_data(body, size_per_chunk, conn)
resp = conn.getresponse()
print(resp.status, resp.reason)
conn.close()
