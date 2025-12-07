package main

import (
	"crypto/ed25519"
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"io/ioutil"
)

func main() {
	// 1. Generate Keypair
	pub, priv, err := ed25519.GenerateKey(rand.Reader)
	if err != nil {
		panic(err)
	}

	// 2. Save Keys
	// Private key is secret!
	if err := ioutil.WriteFile("root.priv", []byte(hex.EncodeToString(priv)), 0600); err != nil {
		panic(err)
	}
	// Public key can be distributed
	if err := ioutil.WriteFile("root.pub", []byte(hex.EncodeToString(pub)), 0644); err != nil {
		panic(err)
	}

	fmt.Println("KEYS GENERATED!")
	fmt.Println("Private Key: root.priv (KEEP SECRET)")
	fmt.Println("Public Key:  root.pub")
	fmt.Println("\n--- GO FORMAT (For RS) ---")
	fmt.Printf("var rootPubKey = ed25519.PublicKey{\n")
	for i, b := range pub {
		fmt.Printf("0x%02x, ", b)
		if (i+1)%8 == 0 {
			fmt.Println()
		}
	}
	fmt.Println("}")

	fmt.Println("\n--- C FORMAT (For Kernel) ---")
	fmt.Printf("static u8int root_pub_key[32] = {\n")
	for i, b := range pub {
		fmt.Printf("0x%02x, ", b)
		if (i+1)%8 == 0 {
			fmt.Println()
		}
	}
	fmt.Println("};")
}
