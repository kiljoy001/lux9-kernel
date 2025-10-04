package p9

import (
	"encoding/binary"
	"fmt"
	"io"
)

// 9P2000 message types
const (
	Tversion = 100
	Rversion = 101
	Tauth    = 102
	Rauth    = 103
	Tattach  = 104
	Rattach  = 105
	Terror   = 106 // illegal
	Rerror   = 107
	Tflush   = 108
	Rflush   = 109
	Twalk    = 110
	Rwalk    = 111
	Topen    = 112
	Ropen    = 113
	Tcreate  = 114
	Rcreate  = 115
	Tread    = 116
	Rread    = 117
	Twrite   = 118
	Rwrite   = 119
	Tclunk   = 120
	Rclunk   = 121
	Tremove  = 122
	Rremove  = 123
	Tstat    = 124
	Rstat    = 125
	Twstat   = 126
	Rwstat   = 127
)

// Qid types
const (
	QTDIR     = 0x80
	QTAPPEND  = 0x40
	QTEXCL    = 0x20
	QTMOUNT   = 0x10
	QTAUTH    = 0x08
	QTTMP     = 0x04
	QTSYMLINK = 0x02
	QTFILE    = 0x00
)

// Open modes
const (
	OREAD  = 0
	OWRITE = 1
	ORDWR  = 2
	OEXEC  = 3
	OTRUNC = 0x10
)

// Dir modes
const (
	DMDIR    = 0x80000000
	DMAPPEND = 0x40000000
	DMEXCL   = 0x20000000
	DMTMP    = 0x04000000
)

const (
	NOTAG uint16 = 0xFFFF
	NOFID uint32 = 0xFFFFFFFF
)

type Qid struct {
	Type uint8
	Vers uint32
	Path uint64
}

type Stat struct {
	Type   uint16
	Dev    uint32
	Qid    Qid
	Mode   uint32
	Atime  uint32
	Mtime  uint32
	Length uint64
	Name   string
	UID    string
	GID    string
	MUID   string
}

type Fcall struct {
	Type uint8
	Tag  uint16
	// Message-specific fields
	Fid    uint32
	Afid   uint32
	Newfid uint32
	Oldtag uint16

	// Tversion/Rversion
	Msize   uint32
	Version string

	// Tattach/Tauth
	Uname string
	Aname string

	// Twalk/Rwalk
	Nwname uint16
	Wname  []string
	Nwqid  uint16
	Wqid   []Qid

	// Topen/Tcreate/Ropen/Rcreate
	Mode   uint8
	Perm   uint32
	Name   string
	Qid    Qid
	Iounit uint32

	// Tread/Rread/Twrite/Rwrite
	Offset uint64
	Count  uint32
	Data   []byte

	// Rerror
	Ename string

	// Tstat/Rstat/Twstat
	Stat Stat
}

func readString(r io.Reader) (string, error) {
	var size uint16
	if err := binary.Read(r, binary.LittleEndian, &size); err != nil {
		return "", err
	}
	buf := make([]byte, size)
	if _, err := io.ReadFull(r, buf); err != nil {
		return "", err
	}
	return string(buf), nil
}

func writeString(w io.Writer, s string) error {
	size := uint16(len(s))
	if err := binary.Write(w, binary.LittleEndian, size); err != nil {
		return err
	}
	_, err := w.Write([]byte(s))
	return err
}

func (q *Qid) Marshal(w io.Writer) error {
	if err := binary.Write(w, binary.LittleEndian, q.Type); err != nil {
		return err
	}
	if err := binary.Write(w, binary.LittleEndian, q.Vers); err != nil {
		return err
	}
	return binary.Write(w, binary.LittleEndian, q.Path)
}

func (q *Qid) Unmarshal(r io.Reader) error {
	if err := binary.Read(r, binary.LittleEndian, &q.Type); err != nil {
		return err
	}
	if err := binary.Read(r, binary.LittleEndian, &q.Vers); err != nil {
		return err
	}
	return binary.Read(r, binary.LittleEndian, &q.Path)
}

func (fc *Fcall) String() string {
	return fmt.Sprintf("Fcall{Type:%d Tag:%d Fid:%d}", fc.Type, fc.Tag, fc.Fid)
}
