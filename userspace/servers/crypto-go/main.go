// Lux9 Crypto Server (Go version)
// Provides cryptographic services via 9P virtual filesystem
package main

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/rand"
	"crypto/sha256"
	"fmt"
	"io"
)

func main() {
	fmt.Println("[crypto] Lux9 Crypto Server starting...")
	fmt.Println("[crypto] Using Go's built-in crypto library")
	fmt.Println("[crypto] Using SIP exchange pages for 9P communication")
	fmt.Println()

	// Test crypto functionality
	testCrypto()

	// Open SIP exchange device
	fmt.Println("[crypto] Opening SIP exchange device...")
	sip, err := OpenExchangeDevice()
	if err != nil {
		fmt.Printf("[crypto] FATAL: Cannot open exchange device: %v\n", err)
		fmt.Println("[crypto] Server cannot start without SIP support")
		return
	}
	defer sip.Close()

	// Read and print exchange status
	status, err := sip.ReadStatus()
	if err != nil {
		fmt.Printf("[crypto] WARNING: Cannot read exchange status: %v\n", err)
	} else {
		fmt.Printf("[crypto] Exchange status:\n%s\n", status)
	}

	// Start 9P server loop
	fmt.Println("[crypto] Starting 9P server via SIP...")
	fmt.Println("[crypto] Waiting for 9P messages on #X/exchange...")

	err = sip.Serve9P(handle9PMessage)
	if err != nil {
		fmt.Printf("[crypto] FATAL: Server loop error: %v\n", err)
	}
}

// Test cryptographic functions
func testCrypto() {
	fmt.Println("[crypto] Testing cryptographic functions...")

	// Test SHA-256
	fmt.Println("[crypto] Testing SHA-256...")
	data := []byte("Hello, Lux9!")
	hash := sha256.Sum256(data)
	fmt.Printf("[crypto]   SHA-256(\"%s\") = %x\n", data, hash)

	// Test AES-256-GCM
	fmt.Println("[crypto] Testing AES-256-GCM...")
	plaintext := []byte("This is a secret message")
	key := make([]byte, 32) // 256-bit key
	nonce := make([]byte, 12) // 96-bit nonce

	// Generate random key and nonce
	rand.Read(key)
	rand.Read(nonce)

	// Encrypt
	ciphertext, err := encryptAESGCM(key, nonce, plaintext, nil)
	if err != nil {
		fmt.Printf("[crypto]   ERROR: %v\n", err)
		return
	}
	fmt.Printf("[crypto]   Encrypted %d bytes → %d bytes (includes 16-byte auth tag)\n",
		len(plaintext), len(ciphertext))

	// Decrypt
	decrypted, err := decryptAESGCM(key, nonce, ciphertext, nil)
	if err != nil {
		fmt.Printf("[crypto]   ERROR: %v\n", err)
		return
	}
	fmt.Printf("[crypto]   Decrypted successfully: \"%s\"\n", decrypted)

	if string(decrypted) != string(plaintext) {
		fmt.Println("[crypto]   ERROR: Decryption mismatch!")
		return
	}

	fmt.Println("[crypto] ✓ All crypto tests passed!")
	fmt.Println()
}

// Encrypt using AES-256-GCM (for secure disk)
func encryptAESGCM(key, nonce, plaintext, additionalData []byte) ([]byte, error) {
	// Create AES cipher
	block, err := aes.NewCipher(key)
	if err != nil {
		return nil, err
	}

	// Create GCM mode
	gcm, err := cipher.NewGCM(block)
	if err != nil {
		return nil, err
	}

	// Encrypt and authenticate
	// This appends the 16-byte authentication tag
	ciphertext := gcm.Seal(nil, nonce, plaintext, additionalData)
	return ciphertext, nil
}

// Decrypt using AES-256-GCM
func decryptAESGCM(key, nonce, ciphertext, additionalData []byte) ([]byte, error) {
	// Create AES cipher
	block, err := aes.NewCipher(key)
	if err != nil {
		return nil, err
	}

	// Create GCM mode
	gcm, err := cipher.NewGCM(block)
	if err != nil {
		return nil, err
	}

	// Decrypt and verify authentication tag
	plaintext, err := gcm.Open(nil, nonce, ciphertext, additionalData)
	if err != nil {
		return nil, err // Authentication failed!
	}

	return plaintext, nil
}

// Future 9P server structure (pseudocode)
/*
type CryptoServer struct {
	// Virtual filesystem structure
	files map[string]*CryptoFile
}

type CryptoFile struct {
	name     string // e.g., "sha256", "aes256gcm"
	category string // e.g., "hash", "aead"
	handler  func([]byte) ([]byte, error)
}

func (s *CryptoServer) serve9P() {
	// Listen for 9P messages on stdin/stdout
	// Handle Topen, Tread, Twrite, etc.
	//
	// Example:
	// - User opens /crypto/hash/sha256
	// - Server returns Qid for sha256 hash file
	// - User writes data → server buffers it
	// - User reads → server computes SHA-256 and returns hash
	//
	// - User opens /crypto/aead/aes256gcm
	// - User writes "key:...|nonce:...|plaintext:..."
	// - User reads → server returns ciphertext+tag
}
*/

// Example crypto handlers that would be called by 9P server

func handleSHA256(data []byte) ([]byte, error) {
	hash := sha256.Sum256(data)
	// Return hex-encoded hash
	return []byte(fmt.Sprintf("%x", hash)), nil
}

func handleAESGCMEncrypt(input []byte) ([]byte, error) {
	// Parse input: "key:...|nonce:...|plaintext:..."
	// (TODO: Implement proper parsing)

	// For now, demonstrate the concept
	key := input[0:32]   // First 32 bytes = key
	nonce := input[32:44] // Next 12 bytes = nonce
	plaintext := input[44:] // Rest = plaintext

	return encryptAESGCM(key, nonce, plaintext, nil)
}

func handleAESGCMDecrypt(input []byte) ([]byte, error) {
	// Parse input: "key:...|nonce:...|ciphertext:..."
	key := input[0:32]
	nonce := input[32:44]
	ciphertext := input[44:]

	return decryptAESGCM(key, nonce, ciphertext, nil)
}

// Random number generation
func handleRandom(n int) ([]byte, error) {
	buf := make([]byte, n)
	_, err := io.ReadFull(rand.Reader, buf)
	if err != nil {
		return nil, err
	}
	return buf, nil
}
