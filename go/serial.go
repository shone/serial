package main

import (
	"github.com/tarm/serial"
	"fmt"
	"flag"
	"os"
	"bufio"
)

var (
	device   = flag.String("device", "/dev/ttyUSB0", "The serial device to use.")
	baudrate = flag.Int("baudrate", 115200, "The baud rate to use on the serial device.")
	verbose  = flag.Bool("verbose", false, "Write verbose logging information to stdout.")
)

func main() {
	flag.Parse()

	serialConfig := &serial.Config{Name: *device, Baud: *baudrate}

	if *verbose {
		fmt.Printf("Connecting to serial device '%s' at baud rate %d...\n", *device, *baudrate)
	}
	serialPort, err := serial.OpenPort(serialConfig)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to open serial port: %s\n", err)
		os.Exit(1)
	}

	defer func() {
		err := serialPort.Close()
		if err != nil {
			fmt.Fprintf(os.Stderr, "Failed to close serial port: %s\n", err)
			os.Exit(1)
		}
		fmt.Println("Closed serial port")
	}()

	stdinInfo, err := os.Stdin.Stat()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error while getting stdin info: %s\n", err)
		os.Exit(1)
	}
	has_stdin := stdinInfo.Mode() & os.ModeNamedPipe != 0

	if has_stdin {
		stdinReader := bufio.NewReader(os.Stdin)
		var writeBuffer [4096]byte
		byte_count, err := stdinReader.Read(writeBuffer[:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error while reading from stdin: %s\n", err)
			os.Exit(1)
		}
		if *verbose {
			fmt.Printf("Got %d bytes on stdin.\n", byte_count)
		}
		_, err = serialPort.Write(writeBuffer[:byte_count])
		if err != nil {
			fmt.Fprintf(os.Stderr, "Failed to write to serial port: %s\n", err)
			os.Exit(1)
		}
	}

	if *verbose {
		fmt.Println("Reading from serial port...")
	}
	var readBuffer [4096]byte
	for {
		bytesRead, err := serialPort.Read(readBuffer[:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "Failed to read from serial port: %s\n", err)
			os.Exit(1)
		}
		fmt.Print(string(readBuffer[:bytesRead]))
	}
}
