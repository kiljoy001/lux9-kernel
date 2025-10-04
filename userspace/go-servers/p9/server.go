package p9

import (
	"encoding/binary"
	"fmt"
	"io"
	"log"
	"sync"
)

// FileServer is the interface that filesystem implementations must satisfy
type FileServer interface {
	// Version negotiates protocol version
	Version(msize uint32, version string) (uint32, string, error)

	// Attach establishes a connection to the filesystem root
	Attach(fid, afid uint32, uname, aname string) (Qid, error)

	// Walk traverses the filesystem
	Walk(fid, newfid uint32, names []string) ([]Qid, error)

	// Open opens a file
	Open(fid uint32, mode uint8) (Qid, uint32, error)

	// Create creates a new file
	Create(fid uint32, name string, perm uint32, mode uint8) (Qid, uint32, error)

	// Read reads from a file
	Read(fid uint32, offset uint64, count uint32) ([]byte, error)

	// Write writes to a file
	Write(fid uint32, offset uint64, data []byte) (uint32, error)

	// Clunk closes a fid
	Clunk(fid uint32) error

	// Remove removes a file
	Remove(fid uint32) error

	// Stat returns file metadata
	Stat(fid uint32) (Stat, error)

	// Wstat changes file metadata
	Wstat(fid uint32, stat Stat) error
}

// Server wraps a FileServer and handles 9P protocol
type Server struct {
	fs    FileServer
	msize uint32
	mu    sync.Mutex
}

func NewServer(fs FileServer) *Server {
	return &Server{
		fs:    fs,
		msize: 8192,
	}
}

// Serve handles 9P protocol on the given reader/writer
func (s *Server) Serve(rw io.ReadWriter) error {
	for {
		fc, err := s.readMsg(rw)
		if err != nil {
			if err == io.EOF {
				return nil
			}
			log.Printf("read error: %v", err)
			return err
		}

		resp := s.dispatch(fc)
		if err := s.writeMsg(rw, resp); err != nil {
			log.Printf("write error: %v", err)
			return err
		}
	}
}

func (s *Server) readMsg(r io.Reader) (*Fcall, error) {
	var size uint32
	if err := binary.Read(r, binary.LittleEndian, &size); err != nil {
		return nil, err
	}

	if size < 7 || size > s.msize {
		return nil, fmt.Errorf("invalid message size: %d", size)
	}

	buf := make([]byte, size-4)
	if _, err := io.ReadFull(r, buf); err != nil {
		return nil, err
	}

	fc := &Fcall{}
	fc.Type = buf[0]
	fc.Tag = binary.LittleEndian.Uint16(buf[1:3])

	// Parse based on message type
	data := buf[3:]
	switch fc.Type {
	case Tversion:
		fc.Msize = binary.LittleEndian.Uint32(data[0:4])
		fc.Version, _ = readString(&byteReader{data[4:]})

	case Tattach:
		fc.Fid = binary.LittleEndian.Uint32(data[0:4])
		fc.Afid = binary.LittleEndian.Uint32(data[4:8])
		r := &byteReader{data[8:]}
		fc.Uname, _ = readString(r)
		fc.Aname, _ = readString(r)

	case Twalk:
		fc.Fid = binary.LittleEndian.Uint32(data[0:4])
		fc.Newfid = binary.LittleEndian.Uint32(data[4:8])
		fc.Nwname = binary.LittleEndian.Uint16(data[8:10])
		r := &byteReader{data[10:]}
		fc.Wname = make([]string, fc.Nwname)
		for i := uint16(0); i < fc.Nwname; i++ {
			fc.Wname[i], _ = readString(r)
		}

	case Topen:
		fc.Fid = binary.LittleEndian.Uint32(data[0:4])
		fc.Mode = data[4]

	case Tcreate:
		fc.Fid = binary.LittleEndian.Uint32(data[0:4])
		r := &byteReader{data[4:]}
		fc.Name, _ = readString(r)
		fc.Perm = binary.LittleEndian.Uint32(r.data[0:4])
		fc.Mode = r.data[4]

	case Tread:
		fc.Fid = binary.LittleEndian.Uint32(data[0:4])
		fc.Offset = binary.LittleEndian.Uint64(data[4:12])
		fc.Count = binary.LittleEndian.Uint32(data[12:16])

	case Twrite:
		fc.Fid = binary.LittleEndian.Uint32(data[0:4])
		fc.Offset = binary.LittleEndian.Uint64(data[4:12])
		fc.Count = binary.LittleEndian.Uint32(data[12:16])
		fc.Data = data[16 : 16+fc.Count]

	case Tclunk, Tremove, Tstat:
		fc.Fid = binary.LittleEndian.Uint32(data[0:4])
	}

	return fc, nil
}

func (s *Server) writeMsg(w io.Writer, fc *Fcall) error {
	buf := &byteBuf{}

	// Write type and tag
	buf.WriteByte(fc.Type)
	buf.WriteU16(fc.Tag)

	// Write message-specific data
	switch fc.Type {
	case Rversion:
		buf.WriteU32(fc.Msize)
		buf.WriteString(fc.Version)

	case Rattach:
		fc.Qid.Marshal(buf)

	case Rwalk:
		buf.WriteU16(fc.Nwqid)
		for i := uint16(0); i < fc.Nwqid; i++ {
			fc.Wqid[i].Marshal(buf)
		}

	case Ropen, Rcreate:
		fc.Qid.Marshal(buf)
		buf.WriteU32(fc.Iounit)

	case Rread:
		buf.WriteU32(fc.Count)
		buf.Write(fc.Data)

	case Rwrite:
		buf.WriteU32(fc.Count)

	case Rclunk, Rremove:
		// No data

	case Rerror:
		buf.WriteString(fc.Ename)

	case Rstat:
		statBuf := &byteBuf{}
		s.writeStat(statBuf, &fc.Stat)
		buf.WriteU16(uint16(len(statBuf.data)))
		buf.Write(statBuf.data)
	}

	// Write size header
	size := uint32(len(buf.data) + 4)
	if err := binary.Write(w, binary.LittleEndian, size); err != nil {
		return err
	}
	_, err := w.Write(buf.data)
	return err
}

func (s *Server) dispatch(fc *Fcall) *Fcall {
	resp := &Fcall{Tag: fc.Tag}

	switch fc.Type {
	case Tversion:
		msize, version, err := s.fs.Version(fc.Msize, fc.Version)
		if err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		s.msize = msize
		resp.Type = Rversion
		resp.Msize = msize
		resp.Version = version

	case Tattach:
		qid, err := s.fs.Attach(fc.Fid, fc.Afid, fc.Uname, fc.Aname)
		if err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Rattach
		resp.Qid = qid

	case Twalk:
		qids, err := s.fs.Walk(fc.Fid, fc.Newfid, fc.Wname)
		if err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Rwalk
		resp.Nwqid = uint16(len(qids))
		resp.Wqid = qids

	case Topen:
		qid, iounit, err := s.fs.Open(fc.Fid, fc.Mode)
		if err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Ropen
		resp.Qid = qid
		resp.Iounit = iounit

	case Tcreate:
		qid, iounit, err := s.fs.Create(fc.Fid, fc.Name, fc.Perm, fc.Mode)
		if err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Rcreate
		resp.Qid = qid
		resp.Iounit = iounit

	case Tread:
		data, err := s.fs.Read(fc.Fid, fc.Offset, fc.Count)
		if err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Rread
		resp.Count = uint32(len(data))
		resp.Data = data

	case Twrite:
		count, err := s.fs.Write(fc.Fid, fc.Offset, fc.Data)
		if err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Rwrite
		resp.Count = count

	case Tclunk:
		if err := s.fs.Clunk(fc.Fid); err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Rclunk

	case Tremove:
		if err := s.fs.Remove(fc.Fid); err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Rremove

	case Tstat:
		stat, err := s.fs.Stat(fc.Fid)
		if err != nil {
			return s.rerror(fc.Tag, err.Error())
		}
		resp.Type = Rstat
		resp.Stat = stat

	default:
		return s.rerror(fc.Tag, "unknown message type")
	}

	return resp
}

func (s *Server) rerror(tag uint16, ename string) *Fcall {
	return &Fcall{
		Type:  Rerror,
		Tag:   tag,
		Ename: ename,
	}
}

func (s *Server) writeStat(buf *byteBuf, st *Stat) {
	buf.WriteU16(st.Type)
	buf.WriteU32(st.Dev)
	st.Qid.Marshal(buf)
	buf.WriteU32(st.Mode)
	buf.WriteU32(st.Atime)
	buf.WriteU32(st.Mtime)
	buf.WriteU64(st.Length)
	buf.WriteString(st.Name)
	buf.WriteString(st.UID)
	buf.WriteString(st.GID)
	buf.WriteString(st.MUID)
}

// Helper types
type byteReader struct {
	data []byte
}

func (r *byteReader) Read(p []byte) (int, error) {
	if len(r.data) == 0 {
		return 0, io.EOF
	}
	n := copy(p, r.data)
	r.data = r.data[n:]
	return n, nil
}

type byteBuf struct {
	data []byte
}

func (b *byteBuf) Write(p []byte) (int, error) {
	b.data = append(b.data, p...)
	return len(p), nil
}

func (b *byteBuf) WriteByte(v byte) {
	b.data = append(b.data, v)
}

func (b *byteBuf) WriteU16(v uint16) {
	b.data = append(b.data, byte(v), byte(v>>8))
}

func (b *byteBuf) WriteU32(v uint32) {
	b.data = append(b.data, byte(v), byte(v>>8), byte(v>>16), byte(v>>24))
}

func (b *byteBuf) WriteU64(v uint64) {
	b.data = append(b.data,
		byte(v), byte(v>>8), byte(v>>16), byte(v>>24),
		byte(v>>32), byte(v>>40), byte(v>>48), byte(v>>56))
}

func (b *byteBuf) WriteString(s string) {
	b.WriteU16(uint16(len(s)))
	b.data = append(b.data, []byte(s)...)
}
