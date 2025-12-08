package main
import (
    "fmt"
    "io/ioutil"
    "os"
)
func main() {
    b, _ := ioutil.ReadFile(os.Args[1])
    fmt.Printf("uchar initcode[] = {\n")
    for i, v := range b {
        fmt.Printf("0x%02x, ", v) 
        if (i+1)%12 == 0 { fmt.Println() }
    }
    fmt.Println("\n};")
}

