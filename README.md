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

[Vídeo demonstrativo do dashboard](https://drive.google.com/file/d/1WPwRLWm8pIGT7dQfDKFMVquZhQgwAq4i/view?usp=sharing)