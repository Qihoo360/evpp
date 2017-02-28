// test project main.go
package client

import (
	"errors"
	"flag"
	"fmt"
	"math/rand"
	"net"
	"os"
	"os/signal"
	"runtime"
	"strconv"
	"sync"
	"time"
)

const (
	ShortConn = iota
	LongConn
)

const (
	SuccessRet = iota
	UnexpectedRet
	ConnErrRet
)

type result struct {
	err        error
	statusCode int
	duration   time.Duration
}

type Evpp struct {
	//N is the total number of requests to make
	N int

	//the number of concurrent short connection workers to run.
	ShortConnectionWorkerNum int

	//the numbner of concurrent long connection workers to run.
	LongConnectionWorkerNum int

	//rate limit
	Qps int

	//address of server
	Address *net.TCPAddr

	//send Buf Size
	BufSize int

	//fill the send buffer
	RandData []byte

	results chan *result
}

func (e *Evpp) checkRecData(data []byte, serial []byte, RecSize int, sendSize int) bool {
	var ret bool
	ret = true
	if sendSize == RecSize {
		for i, v := range serial {
			if v != data[i] {
				ret = false
			}
		}
	} else {
		ret = false
	}
	return ret
}

func (e *Evpp) pingPong(conn *net.TCPConn) {
	s := time.Now()
	serial := strconv.Itoa(rand.Intn(e.N))
	//serial := "1111"
	sendBuf := append([]byte(serial), (e.RandData)...)
	recBuf := make([]byte, len(sendBuf))
	_, err := conn.Write(sendBuf)
	if err == nil {
		length, err := conn.Read(recBuf)
		if err == nil {
			if true == e.checkRecData(recBuf, []byte(serial), length, len(sendBuf)) {
				e.results <- &result{
					err:        err,
					statusCode: SuccessRet,
					duration:   time.Now().Sub(s),
				}
			} else {
				e.results <- &result{
					err:        errors.New("receive unexpected data"),
					statusCode: UnexpectedRet,
					duration:   time.Now().Sub(s),
				}
			}
			return
		}
	}
	e.results <- &result{
		err:        err,
		statusCode: ConnErrRet,
		duration:   time.Now().Sub(s),
	}
}

func (e *Evpp) runWorker(num int, conn *net.TCPConn, conType int) {
	var throttle <-chan time.Time
	if e.Qps > 0 {
		throttle = time.Tick(time.Duration(1e6/(e.Qps)) * time.Microsecond)
	}
	for i := 0; i < num; i++ {
		if e.Qps > 0 {
			<-throttle
		}
		if conType == ShortConn {
			conn_, err := net.DialTCP("tcp", nil, e.Address)
			checkError(err)
			e.pingPong(conn_)
			conn_.Close()
		} else {
			e.pingPong(conn)
		}
	}
}

func (e *Evpp) runWorkers() {
	var wg sync.WaitGroup
	num := e.ShortConnectionWorkerNum + e.LongConnectionWorkerNum
	wg.Add(num)

	for i := 0; i < e.ShortConnectionWorkerNum; i++ {
		go func() {
			e.runWorker(e.N/num, nil, ShortConn)
			wg.Done()
		}()
	}

	for i := 0; i < e.LongConnectionWorkerNum; i++ {
		go func() {
			conn, err := net.DialTCP("tcp", nil, e.Address)
			checkError(err)
			e.runWorker(e.N/num, conn, LongConn)
			defer conn.Close()
			wg.Done()
		}()
	}
	wg.Wait()
	fmt.Println("WaitGroup finished")
}

func (e *Evpp) finalize(duration time.Duration) {
	statusArray := make(map[int]int)
	var cost int64
	for {
		select {
		case res := <-e.results:
			statusArray[res.statusCode]++
			cost += res.duration.Nanoseconds() / 1e6

		default:
			fmt.Fprintf(os.Stdout, "toal test case:%d\n", e.N)
			fmt.Fprintf(os.Stdout, "avg cost time:%dms\n", cost/int64(e.N))
			fmt.Fprintf(os.Stdout, "succ case:%d\n", statusArray[SuccessRet])
			fmt.Fprintf(os.Stdout, "error(received unexpected data) case:%d\n",
				statusArray[UnexpectedRet])
			fmt.Fprintf(os.Stdout, "error(occurred when transaction) case:%d\n",
				statusArray[ConnErrRet])
			return
		}
	}
}

func (e *Evpp) Run() {
	e.results = make(chan *result, e.N)
	start := time.Now()
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)
	go func() {
		<-c
		fmt.Println("receive interrupt signal")
		os.Exit(1)
	}()
	e.runWorkers()
	e.finalize(time.Now().Sub(start))
	close(e.results)
}

func checkError(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error:%s", err.Error())
		os.Exit(1)
	}
}

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())
	var evpp Evpp
	var err error

	var N = flag.Int("N", 20000, "the total number of requests to make")
	var shortConn = flag.Int("shortConnNum", 5,
		"numbers of concurrent short connection workers to run")
	var longConn = flag.Int("longConnNum", 5,
		"numbers of concurrent long connection workers to run")
	var qps = flag.Int("limit", 10000, "rate limit")

	var size = flag.Int("size", 16*1024, "send buf size")

	var addr = flag.String("addr", "127.0.0.1:9099", "<IP:Port>")
	flag.Parse()
	evpp.N = *N
	evpp.ShortConnectionWorkerNum = *shortConn
	evpp.LongConnectionWorkerNum = *longConn
	evpp.Qps = *qps

	evpp.Address, err = net.ResolveTCPAddr("tcp4", *addr)
	checkError(err)

	evpp.BufSize = *size
	evpp.RandData = make([]byte, *size)
	for i := 0; i < *size; i++ {
		evpp.RandData[i] = 'U'
	}

	evpp.Run()
	os.Exit(0)
}
