# Configuração do Servidor Broker Mosquitto

Este diretório contém os arquivos de configuração do servidor broker Mosquitto MQTT.

## Arquivos

### mosquitto.conf
Arquivo de configuração principal do Mosquitto com as seguintes configurações:
- Autenticação obrigatória (`allow_anonymous false`)
- Listener na porta 1883
- Persistência de dados habilitada
- Arquivo de senhas em `/etc/mosquitto/pwfile`

### pwfile
Arquivo de senhas do Mosquitto contendo os usuários autorizados com suas senhas hasheadas.

## Credenciais de Acesso

**Usuário:** `admin`  
**Senha:** `paulvandyk11`

## Instalação

Para aplicar essas configurações no servidor:

1. Copie o arquivo `mosquitto.conf` para `/etc/mosquitto/mosquitto.conf`
2. Copie o arquivo `pwfile` para `/etc/mosquitto/pwfile`
3. Reinicie o serviço Mosquitto:
   ```bash
   sudo systemctl restart mosquitto
   ```

## Testando a Conexão

Para testar a conexão com o broker:

```bash
mosquitto_sub -h <IP_DO_SERVIDOR> -t "sensores/esp32" -u admin -P paulvandyk11
```
