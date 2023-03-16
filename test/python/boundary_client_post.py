
import os, random, sys, requests
from requests_toolbelt.multipart.encoder import MultipartEncoder
 
url = 'http://127.0.0.1:20002/post_boundary'

headers = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:50.0) Gecko/20100101 Firefox/50.0',
    'Referer': url
}

multipart_encoder = MultipartEncoder(
  fields={
    'file': (os.path.basename("test.txt") , open("test.txt", 'rb'), 'text/plain')
    },
    boundary='-----------------------------' + str(random.randint(1e28, 1e29 - 1))
)
headers['Content-Type'] = multipart_encoder.content_type
r = requests.post(url, data=multipart_encoder, headers=headers)
print(r.text)
