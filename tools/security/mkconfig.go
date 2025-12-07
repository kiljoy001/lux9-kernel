package main

import (
	"crypto/ed25519"
	"crypto/sha256"
	"encoding/hex"
	"io/ioutil"
	"log"

	"github.com/fxamacker/cbor/v2"
	"golang.org/x/crypto/blake2b"
)

// Service struct (copy from RS)
type Service struct {
	Name    string   `cbor:"name"`
	Binary  string   `cbor:"bin"`
	Args    []string `cbor:"args,omitempty"`
	Caps    []string `cbor:"caps,omitempty"`
	Hash    []byte   `cbor:"hash,omitempty"`
}

func main() {
	// 1. Load Private Key
	privHex, err := ioutil.ReadFile("root.priv")
	if err != nil {
		log.Fatal("Missing root.priv. Run keygen.go first!")
	}
	privBytes, _ := hex.DecodeString(string(privHex))
	privKey := ed25519.PrivateKey(privBytes)

	// Calculate Hash of driver_console
	consoleBin, err := ioutil.ReadFile("userspace/go_test/driver_console")
	if err != nil {
		log.Fatal("Missing driver_console binary")
	}
	consoleHash := blake2b.Sum512(consoleBin)

	// 2. Define Services
	services := []Service{
		{
			Name:   "Console Driver",
			Binary: "/bin/driver_console",
			Args:   []string{},
			Caps:   []string{"pci", "console"},
			Hash:   consoleHash[:],
		},
	}

	// 3. Encode to CBOR
	data, err := cbor.Marshal(services)
	if err != nil {
		log.Fatal(err)
	}

	// 4. Sign the Hash (SHA256 of the config file itself)
	hash := sha256.Sum256(data)
	sig := ed25519.Sign(privKey, hash[:])

	// 5. Write Output to initrd root
	// Assuming we run this from project root
	ioutil.WriteFile("initrd_root/boot/services.cbor", data, 0644)
	ioutil.WriteFile("initrd_root/boot/services.cbor.sig", sig, 0644)
	
	log.Println("Generated signed config in initrd_root/boot/")
}
