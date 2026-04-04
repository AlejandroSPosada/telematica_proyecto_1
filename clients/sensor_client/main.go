package main

import (
    "bufio"
    "context"
    "fmt"
    "math/rand"
    "net"
    "os"
    "os/signal"
    "strconv"
    "strings"
    "sync"
    "syscall"
    "time"
)

type Config struct {
    Host         string
    Port         int
    SendInterval time.Duration
    BatchSize    int
}

type Sensor struct {
    Type     string
    ID       int
    HasID    bool
    MinValue float64
    MaxValue float64
}

func main() {
    cfg := loadConfig()

    ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
    defer stop()

    sensors := []*Sensor{
        {Type: "temperature", MinValue: 15.0, MaxValue: 35.0},
        {Type: "pressure", MinValue: 950.0, MaxValue: 1050.0},
        {Type: "humidity", MinValue: 30.0, MaxValue: 70.0},
        {Type: "vibration", MinValue: 0.0, MaxValue: 5.0},
        {Type: "energy", MinValue: 100.0, MaxValue: 400.0},
    }

    var wg sync.WaitGroup
    for _, sensor := range sensors {
        wg.Add(1)
        go func(s *Sensor) {
            defer wg.Done()
            s.run(ctx, cfg)
        }(sensor)
    }

    fmt.Println("==================================================")
    fmt.Printf("  Sensores IoT iniciados en Go (%d sensores)\n", len(sensors))
    fmt.Printf("  Servidor: %s:%d\n", cfg.Host, cfg.Port)
    fmt.Println("  Presiona Ctrl+C para detener")
    fmt.Println("==================================================")

    <-ctx.Done()
    fmt.Println("\nDeteniendo sensores...")
    wg.Wait()
    fmt.Println("Sensores detenidos.")
}

func loadConfig() Config {
    return Config{
        Host:         getEnv("SERVER_HOST", "localhost"),
        Port:         getEnvInt("SERVER_PORT", 5000),
        SendInterval: time.Duration(getEnvInt("SEND_INTERVAL", 5)) * time.Second,
        BatchSize:    getEnvInt("BATCH_SIZE", 3),
    }
}

func getEnv(key, fallback string) string {
    value := strings.TrimSpace(os.Getenv(key))
    if value == "" {
        return fallback
    }
    return value
}

func getEnvInt(key string, fallback int) int {
    raw := strings.TrimSpace(os.Getenv(key))
    if raw == "" {
        return fallback
    }
    parsed, err := strconv.Atoi(raw)
    if err != nil || parsed <= 0 {
        return fallback
    }
    return parsed
}

func (s *Sensor) run(ctx context.Context, cfg Config) {
    rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(s.Type[0])*7919))

    for {
        if ctx.Err() != nil {
            return
        }

        conn, reader, err := s.connectAndRegister(cfg)
        if err != nil {
            fmt.Printf("[%s] Error de conexion: %v. Reintentando en 5s...\n", s.Type, err)
            if !sleepContext(ctx, 5*time.Second) {
                return
            }
            continue
        }

        for {
            if ctx.Err() != nil {
                s.disconnect(conn, reader)
                return
            }

            readings := s.generateBatch(rng, cfg.BatchSize)
            if err := s.sendBatch(conn, reader, readings); err != nil {
                fmt.Printf("[%s:%d] Error al enviar batch: %v\n", s.Type, s.ID, err)
                s.closeConn(conn)
                break
            }

            if !sleepContext(ctx, cfg.SendInterval) {
                s.disconnect(conn, reader)
                return
            }
        }
    }
}

func (s *Sensor) connectAndRegister(cfg Config) (net.Conn, *bufio.Reader, error) {
    addr := fmt.Sprintf("%s:%d", cfg.Host, cfg.Port)
    conn, err := net.DialTimeout("tcp", addr, 5*time.Second)
    if err != nil {
        return nil, nil, err
    }

    reader := bufio.NewReader(conn)

    if !s.HasID {
        response, err := s.sendAndReceive(conn, reader, fmt.Sprintf("REGISTER %s\n", s.Type))
        if err != nil {
            conn.Close()
            return nil, nil, err
        }
        fmt.Printf("[%s] REGISTER -> %s\n", s.Type, response)
        tokens := strings.Fields(response)
        if len(tokens) >= 3 && tokens[0] == "RESPONSE" && tokens[1] == "200" {
            id, parseErr := strconv.Atoi(tokens[2])
            if parseErr != nil {
                conn.Close()
                return nil, nil, parseErr
            }
            s.ID = id
            s.HasID = true
            fmt.Printf("[%s] Registrado con ID: %d\n", s.Type, s.ID)
            return conn, reader, nil
        }

        conn.Close()
        return nil, nil, fmt.Errorf("registro fallido: %s", response)
    }

    response, err := s.sendAndReceive(conn, reader, fmt.Sprintf("CONNECT %d %s\n", s.ID, s.Type))
    if err != nil {
        conn.Close()
        return nil, nil, err
    }
    fmt.Printf("[%s:%d] CONNECT -> %s\n", s.Type, s.ID, response)

    tokens := strings.Fields(response)
    if len(tokens) >= 2 && tokens[0] == "RESPONSE" && tokens[1] == "200" {
        return conn, reader, nil
    }

    s.HasID = false
    s.ID = 0
    conn.Close()
    return nil, nil, fmt.Errorf("reconexion rechazada: %s", response)
}

func (s *Sensor) sendBatch(conn net.Conn, reader *bufio.Reader, readings []float64) error {
    timestamp := time.Now().UTC().Format("2006-01-02T15:04:05")
    parts := make([]string, 0, 4+len(readings))
    parts = append(parts, "READ_BATCH", strconv.Itoa(s.ID), timestamp, strconv.Itoa(len(readings)))
    for _, reading := range readings {
        parts = append(parts, fmt.Sprintf("%.2f", reading))
    }

    response, err := s.sendAndReceive(conn, reader, strings.Join(parts, " ")+"\n")
    if err != nil {
        return err
    }
    fmt.Printf("[%s:%d] Batch enviado -> %s\n", s.Type, s.ID, response)
    return nil
}

func (s *Sensor) disconnect(conn net.Conn, reader *bufio.Reader) {
    if conn == nil {
        return
    }
    response, err := s.sendAndReceive(conn, reader, "DISCONNECT\n")
    if err == nil {
        fmt.Printf("[%s:%d] DISCONNECT -> %s\n", s.Type, s.ID, response)
    }
    conn.Close()
}

func (s *Sensor) closeConn(conn net.Conn) {
    if conn != nil {
        _ = conn.Close()
    }
}

func (s *Sensor) sendAndReceive(conn net.Conn, reader *bufio.Reader, message string) (string, error) {
    if _, err := conn.Write([]byte(message)); err != nil {
        return "", err
    }
    response, err := reader.ReadString('\n')
    if err != nil {
        return "", err
    }
    return strings.TrimSpace(response), nil
}

func (s *Sensor) generateBatch(rng *rand.Rand, size int) []float64 {
    readings := make([]float64, 0, size)
    for i := 0; i < size; i++ {
        readings = append(readings, s.generateValue(rng))
    }
    return readings
}

func (s *Sensor) generateValue(rng *rand.Rand) float64 {
    if rng.Float64() < 0.1 {
        switch s.Type {
        case "temperature":
            return round2(rng.Float64()*30 + 50)
        case "pressure":
            return round2(rng.Float64()*100 + 1100)
        case "humidity":
            return round2(rng.Float64()*15 + 85)
        case "vibration":
            return round2(rng.Float64()*7 + 8)
        case "energy":
            return round2(rng.Float64()*300 + 600)
        }
    }

    value := s.MinValue + rng.Float64()*(s.MaxValue-s.MinValue)
    return round2(value)
}

func round2(value float64) float64 {
    return float64(int(value*100+0.5)) / 100
}

func sleepContext(ctx context.Context, duration time.Duration) bool {
    timer := time.NewTimer(duration)
    defer timer.Stop()

    select {
    case <-ctx.Done():
        return false
    case <-timer.C:
        return true
    }
}
