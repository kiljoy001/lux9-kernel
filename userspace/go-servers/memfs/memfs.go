package main

import (
	"fmt"
	"io"
	"lux9/servers/p9"
	"os"
	"sync"
	"time"
)

type MemFS struct {
	mu       sync.Mutex
	fids     map[uint32]*Fid
	root     *File
	pathCount uint64
}

type File struct {
	name     string
	parent   *File
	children map[string]*File
	content  []byte
	stat     p9.Stat
	mu       sync.Mutex
}

type Fid struct {
	file *File
}

func NewMemFS() *MemFS {
	root := &File{
		name:     "/",
		children: make(map[string]*File),
		stat: p9.Stat{
			Type:  0,
			Dev:   0,
			Qid:   p9.Qid{Type: p9.QTDIR, Vers: 0, Path: 0},
			Mode:  p9.DMDIR | 0755,
			Atime: uint32(time.Now().Unix()),
			Mtime: uint32(time.Now().Unix()),
			Name:  "/",
			UID:   "glenda",
			GID:   "glenda",
			MUID:  "glenda",
		},
	}
	root.parent = root // Parent of root is root

	return &MemFS{
		fids:      make(map[uint32]*Fid),
		root:      root,
		pathCount: 1,
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

	fs.fids[fid] = &Fid{file: fs.root}
	return fs.root.stat.Qid, nil
}

func (fs *MemFS) Walk(fid, newfid uint32, names []string) ([]p9.Qid, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return nil, fmt.Errorf("unknown fid")
	}

	curr := f.file
	var qids []p9.Qid

	for _, name := range names {
		if name == ".." {
			curr = curr.parent
		} else {
			child, ok := curr.children[name]
			if !ok {
				return nil, fmt.Errorf("file not found")
			}
			curr = child
		}
		qids = append(qids, curr.stat.Qid)
	}

	fs.fids[newfid] = &Fid{file: curr}
	return qids, nil
}

func (fs *MemFS) Open(fid uint32, mode uint8) (p9.Qid, uint32, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return p9.Qid{}, 0, fmt.Errorf("unknown fid")
	}
	
	// Check permissions? (Skipped for simplicity in this prototype)
	
	return f.file.stat.Qid, 8192, nil
}

func (fs *MemFS) Create(fid uint32, name string, perm uint32, mode uint8) (p9.Qid, uint32, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return p9.Qid{}, 0, fmt.Errorf("unknown fid")
	}

	dir := f.file
	if dir.stat.Mode&p9.DMDIR == 0 {
		return p9.Qid{}, 0, fmt.Errorf("not a directory")
	}

	if _, exists := dir.children[name]; exists {
		return p9.Qid{}, 0, fmt.Errorf("file exists")
	}

	fs.pathCount++
	isDir := (perm & p9.DMDIR) != 0
	qidType := uint8(p9.QTFILE)
	if isDir {
		qidType = p9.QTDIR
	}

	newFile := &File{
		name:     name,
		parent:   dir,
		children: make(map[string]*File),
		stat: p9.Stat{
			Qid:   p9.Qid{Type: qidType, Vers: 0, Path: fs.pathCount},
			Mode:  perm,
			Atime: uint32(time.Now().Unix()),
			Mtime: uint32(time.Now().Unix()),
			Name:  name,
			UID:   dir.stat.UID,
			GID:   dir.stat.GID,
			MUID:  dir.stat.MUID,
		},
	}

	dir.children[name] = newFile
	
	// Update fid to point to the new file (as per 9P spec)
	f.file = newFile

	return newFile.stat.Qid, 8192, nil
}

func (fs *MemFS) Read(fid uint32, offset uint64, count uint32) ([]byte, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return nil, fmt.Errorf("unknown fid")
	}

	file := f.file
	file.mu.Lock()
	defer file.mu.Unlock()

	if file.stat.Mode&p9.DMDIR != 0 {
		// Directory listing
		// TODO: Implement proper directory reading
		// For now, return empty
		return []byte{}, nil
	}

	if offset >= uint64(len(file.content)) {
		return []byte{}, nil
	}

	end := offset + uint64(count)
	if end > uint64(len(file.content)) {
		end = uint64(len(file.content))
	}

	return file.content[offset:end], nil
}

func (fs *MemFS) Write(fid uint32, offset uint64, data []byte) (uint32, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return 0, fmt.Errorf("unknown fid")
	}

	file := f.file
	file.mu.Lock()
	defer file.mu.Unlock()

	if file.stat.Mode&p9.DMDIR != 0 {
		return 0, fmt.Errorf("cannot write to directory")
	}

	// Grow buffer if needed
	end := offset + uint64(len(data))
	if end > uint64(len(file.content)) {
		newContent := make([]byte, end)
		copy(newContent, file.content)
		file.content = newContent
	}

	copy(file.content[offset:], data)
	file.stat.Length = uint64(len(file.content))
	file.stat.Mtime = uint32(time.Now().Unix())
	file.stat.Qid.Vers++

	return uint32(len(data)), nil
}

func (fs *MemFS) Clunk(fid uint32) error {
	fs.mu.Lock()
	defer fs.mu.Unlock()
	delete(fs.fids, fid)
	return nil
}

// secureWipe overwrites the file content with zeros
func (f *File) secureWipe() {
	f.mu.Lock()
	defer f.mu.Unlock()
	
	for i := range f.content {
		f.content[i] = 0
	}
	f.content = nil
}

func (fs *MemFS) Remove(fid uint32) error {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return fmt.Errorf("unknown fid")
	}

	file := f.file
	parent := file.parent

	if parent == file {
		return fmt.Errorf("cannot remove root")
	}

	if len(file.children) > 0 {
		return fmt.Errorf("directory not empty")
	}

	// SECURE WIPE: Overwrite memory before unlinking
	file.secureWipe()

	delete(parent.children, file.name)
	delete(fs.fids, fid) // Clunk implicitly

	return nil
}

func (fs *MemFS) Stat(fid uint32) (p9.Stat, error) {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return p9.Stat{}, fmt.Errorf("unknown fid")
	}

	return f.file.stat, nil
}

func (fs *MemFS) Wstat(fid uint32, stat p9.Stat) error {
	fs.mu.Lock()
	defer fs.mu.Unlock()

	f := fs.fids[fid]
	if f == nil {
		return fmt.Errorf("unknown fid")
	}
	
	// Only update Name for now
	if stat.Name != "" {
		file := f.file
		parent := file.parent
		if parent != nil {
			if _, exists := parent.children[stat.Name]; exists {
				return fmt.Errorf("file exists")
			}
			delete(parent.children, file.name)
			file.name = stat.Name
			parent.children[stat.Name] = file
			file.stat.Name = stat.Name
		}
	}

	return nil
}

func main() {
	fs := NewMemFS()
	srv := p9.NewServer(fs)

	// Serve on Stdin/Stdout (standard Plan 9 service mode)
	if err := srv.Serve(struct{
		io.Reader
		io.Writer
	}{os.Stdin, os.Stdout}); err != nil {
		fmt.Fprintf(os.Stderr, "server error: %v\n", err)
		os.Exit(1)
	}
}
