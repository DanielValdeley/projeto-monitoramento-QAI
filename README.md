# Monitoramento da Qualidade Interna do Ar (QAI)
Código do desenvolvimento

### Placa

> ESP32

### Teste

- Escutar canal mosquitto

```bash
mosquitto_sub -h 192.168.50.20 -t "sensores/temp" -u <usuario> -P <senha>
```

### Resultado DashBoard - Node-RED

![Resultado](/figuras/dashboard-leitura-sensores.png)