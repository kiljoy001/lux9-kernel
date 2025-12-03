package main

import (
	"errors"
	"fmt"
	"hash"
	"sync"

	"lux9/servers/p9"
)

// CryptoFS implements the p9.FileServer interface for crypto operations
type CryptoFS struct {
	mu       sync.Mutex
	fids     map[uint32]*CryptoFid
	nextQid  uint64
	pathMap  map[string]uint64 // path -> qid mapping
}

// CryptoFid represents an open file/operation in the crypto filesystem
type CryptoFid struct {
	qid     p9.Qid
	path    string
	hash    hash.Hash  // Active hash context (for streaming hashes)
	result  []byte     // Cached result after finalization
	offset  uint64     // Read offset for results
	open    bool       // Is the fid open?
	mode    uint8      // Open mode
}

// File type constants
const (
	QTDIR  = 0x80 // Directory
	QTFILE = 0x00 // Regular file
)

// NewCryptoFS creates a new crypto filesystem
func NewCryptoFS() *CryptoFS {
	fs := &CryptoFS{
		fids:    make(map[uint32]*CryptoFid),
		pathMap: make(map[string]uint64),
	}

	// Pre-allocate qids for known paths
	fs.pathMap["/"] = fs.allocQid()
	fs.pathMap["/blake2b"] = fs.allocQid()
	fs.pathMap["/ed25519"] = fs.allocQid()
	fs.pathMap["/ed25519/verify"] = fs.allocQid()

	return fs
}

func (fs *CryptoFS) allocQid() uint64 {
	qid := fs.nextQid
	fs.nextQid++
	return qid
}

func (fs *CryptoFS) getQid(path string) (p9.Qid, error) {
	qid, ok := fs.pathMap[path]
	if !ok {
		return p9.Qid{}, errors.New("path not found")
	}

	qtype := QTFILE
	if path == "/" || path == "/ed25519" {
		qtype = QTDIR
	}

	return p9.Qid{
		Type: uint8(qtype),
		Vers: 0,
		Path: qid,
	}, nil
}

// Version negotiates protocol version
func (fs *CryptoFS) Version(msize uint32, version string) (uint32, string, error) {
	if version != "9P2000" {
		return 0, "", errors.New("unsupported protocol version")
	}
	return msize, "9P2000", nil
}

// Attach establishes a connection to the filesystem root
func (fs *CryptoFS) Attach(fid, afid uint32, uname, aname string) (p9.Qid, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	qid, err := fs.getQid("/")
	if err != nil {
		return p9.Qid{}, err
	}

	fs.fids[fid] = &CryptoFid{
		qid:  qid,
		path: "/",
	}

	return qid, nil
}

// Walk traverses the filesystem tree
func (fs *CryptoFS) Walk(fid, newfid uint32, names []string) ([]p9.Qid, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	cf, ok := fs.fids[fid]
	if !ok {
		return nil, errors.New("unknown fid")
	}

	// Walk from current path
	currentPath := cf.path
	qids := make([]p9.Qid, 0, len(names))

	for _, name := range names {
		// Construct next path
		nextPath := currentPath
		if currentPath == "/" {
			nextPath = "/" + name
		} else {
			nextPath = currentPath + "/" + name
		}

		qid, err := fs.getQid(nextPath)
		if err != nil {
			return qids, err
		}

		qids = append(qids, qid)
		currentPath = nextPath
	}

	// Create new fid or update existing
	if fid == newfid {
		cf.path = currentPath
		cf.qid = qids[len(qids)-1]
	} else {
		fs.fids[newfid] = &CryptoFid{
			qid:  qids[len(qids)-1],
			path: currentPath,
		}
	}

	return qids, nil
}

// Open opens a file
func (fs *CryptoFS) Open(fid uint32, mode uint8) (p9.Qid, uint32, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	cf, ok := fs.fids[fid]
	if !ok {
		return p9.Qid{}, 0, errors.New("unknown fid")
	}

	cf.open = true
	cf.mode = mode
	cf.offset = 0

	// Initialize hash context if it's a hash file
	if cf.path == "/blake2b" {
		cf.hash = NewBlake2bHash()
		cf.result = nil
	}

	return cf.qid, 8192, nil // Return 8KB iounit
}

// Create is not supported (read-only crypto operations)
func (fs *CryptoFS) Create(fid uint32, name string, perm uint32, mode uint8) (p9.Qid, uint32, error) {
	return p9.Qid{}, 0, errors.New("create not supported")
}

// Read reads from a file
func (fs *CryptoFS) Read(fid uint32, offset uint64, count uint32) ([]byte, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	cf, ok := fs.fids[fid]
	if !ok {
		return nil, errors.New("unknown fid")
	}

	if !cf.open {
		return nil, errors.New("fid not open")
	}

	// Directory reads
	if cf.qid.Type&QTDIR != 0 {
		return fs.readDir(cf, offset, count)
	}

	// File reads - return finalized result
	if cf.result == nil {
		// Finalize hash if not yet done
		if cf.hash != nil {
			cf.result = cf.hash.Sum(nil)
		} else {
			return nil, errors.New("no data available")
		}
	}

	// Return data from offset
	if offset >= uint64(len(cf.result)) {
		return []byte{}, nil
	}

	end := offset + uint64(count)
	if end > uint64(len(cf.result)) {
		end = uint64(len(cf.result))
	}

	return cf.result[offset:end], nil
}

// readDir returns directory entries
func (fs *CryptoFS) readDir(cf *CryptoFid, offset uint64, count uint32) ([]byte, error) {
	var entries []p9.Stat

	switch cf.path {
	case "/":
		entries = []p9.Stat{
			{Name: "blake2b", Qid: p9.Qid{Type: QTFILE, Path: fs.pathMap["/blake2b"]}, Mode: 0666},
			{Name: "ed25519", Qid: p9.Qid{Type: QTDIR, Path: fs.pathMap["/ed25519"]}, Mode: 0777 | 0x80000000},
		}
	case "/ed25519":
		entries = []p9.Stat{
			{Name: "verify", Qid: p9.Qid{Type: QTFILE, Path: fs.pathMap["/ed25519/verify"]}, Mode: 0666},
		}
	}

	// Serialize stat entries (simplified - real implementation would use convS2M)
	data := make([]byte, 0, count)
	for _, stat := range entries {
		statBytes := serializeStat(stat)
		if uint32(len(data)+len(statBytes)) > count {
			break
		}
		data = append(data, statBytes...)
	}

	return data, nil
}

// Write writes data to a file (for hash input)
func (fs *CryptoFS) Write(fid uint32, offset uint64, data []byte) (uint32, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	cf, ok := fs.fids[fid]
	if !ok {
		return 0, errors.New("unknown fid")
	}

	if !cf.open {
		return 0, errors.New("fid not open")
	}

	// Stream data into hash
	if cf.hash != nil {
		n, err := cf.hash.Write(data)
		return uint32(n), err
	}

	// Ed25519 verification
	if cf.path == "/ed25519/verify" {
		result, err := VerifyEd25519(data)
		if err != nil {
			return 0, err
		}
		cf.result = []byte(result)
		return uint32(len(data)), nil
	}

	return 0, errors.New("write not supported for this file")
}

// Clunk closes a fid
func (fs *CryptoFS) Clunk(fid uint32) error {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	delete(fs.fids, fid)
	return nil
}

// Remove is not supported
func (fs *CryptoFS) Remove(fid uint32) error {
	return errors.New("remove not supported")
}

// Stat returns file metadata
func (fs *CryptoFS) Stat(fid uint32) (p9.Stat, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	cf, ok := fs.fids[fid]
	if !ok {
		return p9.Stat{}, errors.New("unknown fid")
	}

	mode := uint32(0666)
	if cf.qid.Type&QTDIR != 0 {
		mode = 0777 | 0x80000000
	}

	return p9.Stat{
		Name:   getFileName(cf.path),
		Qid:    cf.qid,
		Mode:   mode,
		Length: uint64(len(cf.result)),
	}, nil
}

// Wstat is not supported
func (fs *CryptoFS) Wstat(fid uint32, stat p9.Stat) error {
	return errors.New("wstat not supported")
}

// Helper functions

func getFileName(path string) string {
	if path == "/" {
		return "/"
	}
	// Find last slash
	for i := len(path) - 1; i >= 0; i-- {
		if path[i] == '/' {
			return path[i+1:]
		}
	}
	return path
}

func serializeStat(stat p9.Stat) []byte {
	// Simplified stat serialization
	// Real implementation would use proper 9P stat encoding
	result := fmt.Sprintf("%s\n", stat.Name)
	return []byte(result)
}
