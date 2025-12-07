package main

import (
	"log"
	"net/http"
)

func main() {
	log.Println("WebServer: Starting...")
	
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte("Hello from Secure Service!"))
	})

	log.Fatal(http.ListenAndServe(":80", nil))
}
