// mp.go — phép thử MP trong Go: biến thường vs sync/atomic.
// Mỗi lượt hai goroutine (khóa vào luồng OS, ghim vào CPU 0 và 1) gặp nhau ở một hàng rào quay;
// người ghi: x = 1; y = 1; người đọc: r0 = y; r1 = x. Yếu: r0 == 1 && r1 == 0.
// chạy: go run mp.go <plain|atomic> <số lượt>
package main

import (
	"fmt"
	"os"
	"runtime"
	"strconv"
	"sync"
	"sync/atomic"
	"syscall"
	"unsafe"
)

const B = 4096

type cell struct {
	v int32
	_ [60]byte
}

var X, Y [B]cell
var res [B]int32
var bar atomic.Int64

func pin(cpu int) {
	runtime.LockOSThread()
	var mask [16]uint64
	mask[cpu/64] |= 1 << (cpu % 64)
	_, _, e := syscall.RawSyscall(syscall.SYS_SCHED_SETAFFINITY, 0, uintptr(len(mask)*8), uintptr(unsafe.Pointer(&mask[0])))
	if e != 0 {
		panic(e)
	}
}

func barrier(k int64) {
	bar.Add(1)
	for bar.Load() < 2*k {
	}
}

func main() {
	variant := os.Args[1]
	n, _ := strconv.ParseInt(os.Args[2], 10, 64)
	batches := (n + B - 1) / B
	var hist [4]int64
	k := int64(0)
	for b := int64(0); b < batches; b++ {
		for i := range X {
			X[i].v, Y[i].v = 0, 0
		}
		var wg sync.WaitGroup
		wg.Add(1)
		k0 := k
		go func() {
			defer wg.Done()
			pin(0)
			kk := k0
			for i := 0; i < B; i++ {
				kk++
				barrier(kk)
				if variant == "atomic" {
					atomic.StoreInt32(&X[i].v, 1)
					atomic.StoreInt32(&Y[i].v, 1)
				} else {
					X[i].v = 1
					Y[i].v = 1
				}
			}
		}()
		done := make(chan int64)
		go func() {
			pin(1)
			kk := k0
			for i := 0; i < B; i++ {
				kk++
				barrier(kk)
				var r0, r1 int32
				if variant == "atomic" {
					r0 = atomic.LoadInt32(&Y[i].v)
					r1 = atomic.LoadInt32(&X[i].v)
				} else {
					r0 = Y[i].v
					r1 = X[i].v
				}
				res[i] = r0*2 + r1
			}
			done <- kk
		}()
		k = <-done
		wg.Wait()
		for i := 0; i < B; i++ {
			hist[res[i]]++
		}
	}
	total := batches * B
	fmt.Printf("go %s %-7s n=%d  yếu: %d (%.3g /triệu)  r0r1: 00=%d 01=%d 10=%d* 11=%d\n", runtime.GOARCH, variant,
		total, hist[2], 1e6*float64(hist[2])/float64(total), hist[0], hist[1], hist[2], hist[3])
}
