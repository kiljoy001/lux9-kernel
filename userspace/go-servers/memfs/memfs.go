package main

import (
	"fmt"
	"lux9/servers/p9"
	"os"
	"sync"
)

type MemFS struct {
	mu   sync.Mutex
	fids map[uint32]*Fid
}

type Fid struct {
	path string
	qid  p9.Qid
}

func NewMemFS() *MemFS {
	return &MemFS{
		fids: make(map[uint32]*Fid),
	}
}

func (fs *MemFS) Version(msize uint32, version string) (uint32, string, error) {
	if version != "9P2000" {
		return 0, "", fmt.Errorf("unsupported version")
	}
	if msize > 8192 {
		msize = 8192
	}
	return msize, "9P2000", nil
}

func (fs *MemFS) Attach(fid, afid uint32, uname, aname string) (p9.Qid, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	qid := p9.Qid{Type: p9.QTDIR, Path: 1, Vers: 0}
	fs.fids[fid] = &Fid{path: "/", qid: qid}
	return qid, nil
}

func (fs *MemFS) Walk(fid, newfid uint32, names []string) ([]p9.Qid, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return nil, fmt.Errorf("unknown fid")
	}

	// Simple implementation - just return same qid
	qids := []p9.Qid{f.qid}
	fs.fids[newfid] = &Fid{path: f.path, qid: f.qid}
	return qids, nil
}

func (fs *MemFS) Open(fid uint32, mode uint8) (p9.Qid, uint32, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return p9.Qid{}, 0, fmt.Errorf("unknown fid")
	}
	return f.qid, 8192, nil
}

func (fs *MemFS) Create(fid uint32, name string, perm uint32, mode uint8) (p9.Qid, uint32, error) {
	return p9.Qid{}, 0, fmt.Errorf("not implemented")
}

func (fs *MemFS) Read(fid uint32, offset uint64, count uint32) ([]byte, error) {
	return []byte("Hello from MemFS\n"), nil
}

func (fs *MemFS) Write(fid uint32, offset uint64, data []byte) (uint32, error) {
	return 0, fmt.Errorf("not implemented")
}

func (fs *MemFS) Clunk(fid uint32) error {
	fs.mu.Lock()
	defer fs.mu.Unlock()
	delete(fs.fids, fid)
	return nil
}

func (fs *MemFS) Remove(fid uint32) error {
	return fmt.Errorf("not implemented")
}

func (fs *MemFS) Stat(fid uint32) (p9.Stat, error) {
	return p9.Stat{}, fmt.Errorf("not implemented")
}

func (fs *MemFS) Wstat(fid uint32, stat p9.Stat) error {
	return fmt.Errorf("not implemented")
}

func main() {
	fs := NewMemFS()
	srv := p9.NewServer(fs)

	if err := srv.Serve(os.Stdin); err != nil {
		fmt.Fprintf(os.Stderr, "server error: %v\n", err)
		os.Exit(1)
	}
}
