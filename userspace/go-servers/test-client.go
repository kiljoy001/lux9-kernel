package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"os/exec"
)

func main() {
	// Start memfs server
	cmd := exec.Command("./memfs")
	stdin, _ := cmd.StdinPipe()
	stdout, _ := cmd.StdoutPipe()
	cmd.Start()

	// Send Tversion
	sendTversion(stdin, 8192, "9P2000")
	readRversion(stdout)

	// Send Tattach
	sendTattach(stdin, 1, 0xFFFFFFFF, "user", "")
	readRattach(stdout)

	// Send Topen
	sendTopen(stdin, 1, 0)
	readRopen(stdout)

	// Send Tread
	sendTread(stdin, 1, 0, 100)
	data := readRread(stdout)
	fmt.Printf("Read: %s\n", string(data))

	// Send Tclunk
	sendTclunk(stdin, 1)
	readRclunk(stdout)

	stdin.Close()
	cmd.Wait()
}

func sendTversion(w io.Writer, msize uint32, version string) {
	buf := make([]byte, 4+1+2+4+2+len(version))
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 100 // Tversion
	binary.LittleEndian.PutUint16(buf[5:7], 0xFFFF)
	binary.LittleEndian.PutUint32(buf[7:11], msize)
	binary.LittleEndian.PutUint16(buf[11:13], uint16(len(version)))
	copy(buf[13:], version)
	w.Write(buf)
}

func readRversion(r io.Reader) {
	var size uint32
	binary.Read(r, binary.LittleEndian, &size)
	buf := make([]byte, size-4)
	io.ReadFull(r, buf)
	fmt.Printf("Rversion: msize=%d\n", binary.LittleEndian.Uint32(buf[3:7]))
}

func sendTattach(w io.Writer, fid, afid uint32, uname, aname string) {
	size := 4 + 1 + 2 + 4 + 4 + 2 + len(uname) + 2 + len(aname)
	buf := make([]byte, size)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(size))
	buf[4] = 104 // Tattach
	binary.LittleEndian.PutUint16(buf[5:7], 1)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	binary.LittleEndian.PutUint32(buf[11:15], afid)
	binary.LittleEndian.PutUint16(buf[15:17], uint16(len(uname)))
	copy(buf[17:], uname)
	off := 17 + len(uname)
	binary.LittleEndian.PutUint16(buf[off:off+2], uint16(len(aname)))
	copy(buf[off+2:], aname)
	w.Write(buf)
}

func readRattach(r io.Reader) {
	var size uint32
	binary.Read(r, binary.LittleEndian, &size)
	buf := make([]byte, size-4)
	io.ReadFull(r, buf)
	fmt.Printf("Rattach received\n")
}

func sendTopen(w io.Writer, fid uint32, mode byte) {
	buf := make([]byte, 4+1+2+4+1)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 112 // Topen
	binary.LittleEndian.PutUint16(buf[5:7], 2)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	buf[11] = mode
	w.Write(buf)
}

func readRopen(r io.Reader) {
	var size uint32
	binary.Read(r, binary.LittleEndian, &size)
	buf := make([]byte, size-4)
	io.ReadFull(r, buf)
	fmt.Printf("Ropen received\n")
}

func sendTread(w io.Writer, fid uint32, offset uint64, count uint32) {
	buf := make([]byte, 4+1+2+4+8+4)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 116 // Tread
	binary.LittleEndian.PutUint16(buf[5:7], 3)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	binary.LittleEndian.PutUint64(buf[11:19], offset)
	binary.LittleEndian.PutUint32(buf[19:23], count)
	w.Write(buf)
}

func readRread(r io.Reader) []byte {
	var size uint32
	binary.Read(r, binary.LittleEndian, &size)
	buf := make([]byte, size-4)
	io.ReadFull(r, buf)
	count := binary.LittleEndian.Uint32(buf[3:7])
	return buf[7 : 7+count]
}

func sendTclunk(w io.Writer, fid uint32) {
	buf := make([]byte, 4+1+2+4)
	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(buf)))
	buf[4] = 120 // Tclunk
	binary.LittleEndian.PutUint16(buf[5:7], 4)
	binary.LittleEndian.PutUint32(buf[7:11], fid)
	w.Write(buf)
}

func readRclunk(r io.Reader) {
	var size uint32
	binary.Read(r, binary.LittleEndian, &size)
	buf := make([]byte, size-4)
	io.ReadFull(r, buf)
	fmt.Printf("Rclunk received\n")
}
