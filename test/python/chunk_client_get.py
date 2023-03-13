import http.client

conn = http.client.HTTPConnection('127.0.0.1', 20002)
conn.request('GET', "/get_chunk")
resp = conn.getresponse()
resp.chunked = False

def get_chunk_size():
    size_str = resp.read(2)
    while size_str[-2:] != b"\r\n":
        size_str += resp.read(1)
    return int(size_str[:-2], 16)

def get_chunk_data(chunk_size):
    data = resp.read(chunk_size)
    resp.read(2)
    return data

respbody = ""
while True:
    chunk_size = get_chunk_size()
    if (chunk_size == 0):
        break
    else:
        chunk_data = get_chunk_data(chunk_size)
        print("Chunk Received: " + chunk_data.decode())
        respbody += chunk_data.decode()

conn.close()
print(respbody)