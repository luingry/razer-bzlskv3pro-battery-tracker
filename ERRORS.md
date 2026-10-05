# Erros tratados

Consultar este arquivo antes de investigar um problema relacionado. Acrescentar somente erros não triviais resolvidos, com evidência e prevenção.

## HID: resposta válida rejeitada como incompleta (1.0.0)

- **Sintoma:** o Basilisk V3 Pro 35K conectado era encontrado, mas a consulta retornava bateria indisponível e erro Windows 13 (`ERROR_INVALID_DATA`).
- **Causa:** o buffer contém 91 bytes (report ID zero + payload de 90 bytes), mas alguns drivers HID do Windows retornam `bytesReturned = 90`, sem contar o report ID. A verificação exigia 91.
- **Solução:** aceitar 90 somente quando o report ID é zero, ou 91; continuar validando status, transação, comando, tamanho e checksum do payload. Transferências menores permanecem inválidas.
- **Prova:** diagnóstico físico no dispositivo `1532:00CD`: 100%, charging=0, win32Error=0. Teste de regressão cobre os dois comprimentos e rejeita report ID/comprimentos incompatíveis.
- **Prevenção:** distinguir tamanho do buffer de tamanho reportado pela API. Referência: [HIDAPI Windows, hid_get_report](https://github.com/libusb/hidapi/blob/master/windows/hid.c). Não remover a validação do pacote para contornar diferenças de transporte.
