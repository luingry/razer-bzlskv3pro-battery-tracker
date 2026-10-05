# Erros tratados

Consultar este arquivo antes de investigar um problema relacionado. Acrescentar somente erros não triviais resolvidos, com evidência e prevenção.

## HID: resposta válida rejeitada como incompleta (1.0.0)

- **Sintoma:** o Basilisk V3 Pro 35K conectado era encontrado, mas a consulta retornava bateria indisponível e erro Windows 13 (`ERROR_INVALID_DATA`).
- **Causa:** o buffer contém 91 bytes (report ID zero + payload de 90 bytes), mas alguns drivers HID do Windows retornam `bytesReturned = 90`, sem contar o report ID. A verificação exigia 91.
- **Solução:** aceitar 90 somente quando o report ID é zero, ou 91; continuar validando status, transação, comando, tamanho e checksum do payload. Transferências menores permanecem inválidas.
- **Prova:** diagnóstico físico no dispositivo `1532:00CD`: 100%, charging=0, win32Error=0. Teste de regressão cobre os dois comprimentos e rejeita report ID/comprimentos incompatíveis.
- **Prevenção:** distinguir tamanho do buffer de tamanho reportado pela API. Referência: [HIDAPI Windows, hid_get_report](https://github.com/libusb/hidapi/blob/master/windows/hid.c). Não remover a validação do pacote para contornar diferenças de transporte.

## Instalador: upgrade silencioso cancelado com app aberto (1.0.2)

- **Sintoma:** setup publicado retornava código 1 no upgrade; log mostrava a mensagem de app em execução e `EAbort`, antes da cópia de arquivos.
- **Causa:** Inno Setup verifica `AppMutex` antes de executar `PrepareToInstall`. A rotina de fechamento estava somente nesse evento, tarde demais para a verificação inicial.
- **Solução:** fechar o app em `InitializeSetup`, usando `WM_CLOSE` para a janela oculta, e aguardar até quatro segundos pela liberação do mutex; manter a verificação de instância ativa e o fechamento antes da cópia/desinstalação.
- **Prevenção:** testar instalador com a versão anterior realmente em execução, incluindo modo silencioso e preservação do valor de inicialização. `scripts/test-installer.ps1` executa esse fluxo localmente e no CI antes de publicar.
- **Referência:** [ordem dos eventos no código do Inno Setup](https://github.com/jrsoftware/issrc/blob/main/Projects/Src/Setup.MainFunc.pas), `InitializeSetup` antes de `CheckForMutexes(ExpandedAppMutex)`.

## Script de teste: recriação de chave compartilhada do registro (1.0.2)

- **Sintoma:** após o primeiro smoke test do instalador, a chave Run do usuário perdeu entradas de outros aplicativos.
- **Causa:** `New-Item -Force` sobre uma chave de registro existente recria a chave; ele não tem a mesma semântica de garantir uma pasta existente. A preparação do teste usava essa operação na chave compartilhada.
- **Solução:** criar a chave somente se ela não existe, modificar/remover apenas o valor pertencente ao produto e comparar snapshot de nomes/tipos/valores antes e depois do teste.
- **Prova:** 11 entradas recuperadas de uma captura do provedor WMI de startup, conferidas individualmente; novo teste com versão em execução preservou as 11 entradas e a preferência temporária do produto. Backup local guardado fora do Git.
- **Prevenção:** nunca usar `New-Item -Force` em chaves compartilhadas do registro. Testes de configuração devem preservar valores não relacionados e verificar isso explicitamente antes de publicar. Não versionar backups locais que contenham configurações pessoais.
