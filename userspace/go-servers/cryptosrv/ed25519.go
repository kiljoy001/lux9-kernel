package main

import (
	"crypto/ed25519"
	"encoding/hex"
	"errors"
	"fmt"
	"strings"
)

// VerifyEd25519 verifies an Ed25519 signature
// Input format: "pubkey:signature:message" (all hex-encoded)
// Returns: "OK" or "FAIL"
func VerifyEd25519(input []byte) (string, error) {
	// Parse input: pubkey:signature:message
	parts := strings.Split(string(input), ":")
	if len(parts) != 3 {
		return "", errors.New("invalid format: expected pubkey:signature:message")
	}

	pubkeyHex := strings.TrimSpace(parts[0])
	sigHex := strings.TrimSpace(parts[1])
	messageHex := strings.TrimSpace(parts[2])

	// Decode public key (32 bytes)
	pubkey, err := hex.DecodeString(pubkeyHex)
	if err != nil {
		return "", fmt.Errorf("invalid pubkey hex: %w", err)
	}
	if len(pubkey) != ed25519.PublicKeySize {
		return "", fmt.Errorf("invalid pubkey size: got %d, want %d", len(pubkey), ed25519.PublicKeySize)
	}

	// Decode signature (64 bytes)
	sig, err := hex.DecodeString(sigHex)
	if err != nil {
		return "", fmt.Errorf("invalid signature hex: %w", err)
	}
	if len(sig) != ed25519.SignatureSize {
		return "", fmt.Errorf("invalid signature size: got %d, want %d", len(sig), ed25519.SignatureSize)
	}

	// Decode message
	message, err := hex.DecodeString(messageHex)
	if err != nil {
		return "", fmt.Errorf("invalid message hex: %w", err)
	}

	// Verify signature
	if ed25519.Verify(pubkey, message, sig) {
		return "OK\n", nil
	}

	return "FAIL\n", nil
}

// SignEd25519 signs a message with an Ed25519 private key
// This is primarily for testing; production signing should be done host-side
// Input format: "privkey:message" (hex-encoded)
// Returns: signature (hex-encoded)
func SignEd25519(input []byte) (string, error) {
	// Parse input: privkey:message
	parts := strings.Split(string(input), ":")
	if len(parts) != 2 {
		return "", errors.New("invalid format: expected privkey:message")
	}

	privkeyHex := strings.TrimSpace(parts[0])
	messageHex := strings.TrimSpace(parts[1])

	// Decode private key (64 bytes)
	privkey, err := hex.DecodeString(privkeyHex)
	if err != nil {
		return "", fmt.Errorf("invalid privkey hex: %w", err)
	}
	if len(privkey) != ed25519.PrivateKeySize {
		return "", fmt.Errorf("invalid privkey size: got %d, want %d", len(privkey), ed25519.PrivateKeySize)
	}

	// Decode message
	message, err := hex.DecodeString(messageHex)
	if err != nil {
		return "", fmt.Errorf("invalid message hex: %w", err)
	}

	// Sign message
	sig := ed25519.Sign(privkey, message)

	return hex.EncodeToString(sig) + "\n", nil
}

// GenerateEd25519Keypair generates a new Ed25519 keypair
// Returns: "pubkey:privkey" (hex-encoded)
func GenerateEd25519Keypair() string {
	pubkey, privkey, err := ed25519.GenerateKey(nil)
	if err != nil {
		return fmt.Sprintf("ERROR: %v\n", err)
	}

	return fmt.Sprintf("%s:%s\n",
		hex.EncodeToString(pubkey),
		hex.EncodeToString(privkey))
}
