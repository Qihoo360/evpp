import sys
sys.path.append('gen-py')

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from echo.ttypes import Request, Response
from echo import Echo


def echo(s, client):
    s = client.echo(s)
    print(s)
    return s

def execute(client):
    r = Request(101, 102, "ccc", "ddd", "comments")
    result = client.execute("python-exec", r)
    print("execute finished")
    return result

def ping(client):
    client.ping()
    print("ping ...")


def main():
    transport = TSocket.TSocket('127.0.0.1', 9099)
    tranport = TTransport.TFramedTransport(transport)
    protocol = TBinaryProtocol.TBinaryProtocol(tranport)
    client = Echo.Client(protocol)
    tranport.open()
    echo("42", client)
    ping(client);
    execute(client);
    tranport.close()


if __name__ == '__main__':
    main()
