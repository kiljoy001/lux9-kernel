package main

import (
	"hash"

	"golang.org/x/crypto/blake2b"
)

// NewBlake2bHash creates a new Blake2b-512 hash instance
func NewBlake2bHash() hash.Hash {
	// Blake2b-512 (64 bytes output)
	h, err := blake2b.New512(nil)
	if err != nil {
		panic("blake2b.New512 failed: " + err.Error())
	}
	return h
}

// Blake2b512 computes a Blake2b-512 hash of the input data
// This is a convenience function for one-shot hashing
func Blake2b512(data []byte) []byte {
	sum := blake2b.Sum512(data)
	return sum[:]
}

// Blake2b256 computes a Blake2b-256 hash of the input data
// Useful for shorter hash outputs
func Blake2b256(data []byte) []byte {
	sum := blake2b.Sum256(data)
	return sum[:]
}
