import dpkt
import base64

file = open('hehe.pcap', 'rb')
pcap = dpkt.pcap.Reader(file)

max_ttl = -1
payload = ''

for ts, buf in pcap:
    eth = dpkt.ethernet.Ethernet(buf)
    ip = eth.data
    tcp = ip.data

    if ip.ttl > max_ttl:
        max_ttl = ip.ttl
        payload = tcp.data

print(max_ttl)
print(base64.b64decode(payload))