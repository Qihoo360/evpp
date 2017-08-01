namespace cpp echo
namespace py echo

struct Request {
  1: i32 num1 = 0,
  2: i32 num2,
  3: string c,
  4: string d,
  5: optional string comment,
}

struct Response {
  1: i32 result,
  2: string x,
  3: string y,
  4: optional string z,
}

service Echo
{
  string echo(1: string arg);
  Response execute(1: string name, 2: Request r);
  void ping();
}

