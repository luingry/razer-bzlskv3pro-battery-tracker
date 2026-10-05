# Especificação do Basilisk Battery

## Experiência

- App simples, sem janela principal, na área de notificações do Windows.
- Ícone de bateria com percentual dentro do próprio ícone; tooltip informa modelo, conexão, percentual e carregamento quando disponível.
- Fonte Segoe UI Semibold, altura nominal proporcional de 10/16 do canvas, centralizada com padding horizontal; fonte/altura do menu seguem as preferências nativas do Windows.
- Verde Razer `#44D62C` a partir de 50%; amarelo `#FFCD35` de 25% a 49%; vermelho `#FF4C4C` abaixo de 25%.
- Em 100%, substituir o número por marca de concluído (check). Ausência/erro de leitura usa bateria cinza com `?`; nunca inventar 0% ou manter silenciosamente uma leitura antiga.
- Menu em português: estado, “Iniciar com o Windows” (marcado quando ativo), “Atualizar agora”, nome/versão e “Sair”.
- Inicialização desativada na primeira instalação; escolha explícita pelo menu. Preferência preservada em upgrades e removida ao desinstalar.
- Ícone do aplicativo/instalador gerado pelo ChatGPT, com transparência real. O ícone dinâmico da bandeja é desenhado com GDI nativo e mantém alfa, sem baixar recursos em runtime.

## Hardware

- Razer VID `1532`, Basilisk V3 Pro USB `00AA`/receptor `00AB`, V3 Pro 35K USB `00CC`/receptor `00CD`.
- Descoberta automática HID, preferência pelo cabo quando cabo e receptor aparecem juntos, nova descoberta em eventos de conexão, retomada ou falha.
- Wireless significa receptor USB de 2,4 GHz neste escopo. Bluetooth e Mouse Dock Pro não fazem parte da compatibilidade confirmada; requerem transporte/protocolo próprios e validação física antes de prometer suporte.
- Consultar somente bateria (`07/80`) e carregamento (`07/84`); não escrever DPI, iluminação ou configurações do mouse. Não requer Synapse, driver novo ou acesso de administrador.

## Desempenho e estabilidade

- C++/Win32 x64; somente DLLs do Windows, sem runtime adicional, processo auxiliar ou rede.
- Intervalo de 30 segundos; eventos disparam atualização, consultas coalescidas em um worker que dorme entre trabalhos.
- Caminho HID e ícone são reaproveitados; redesenhar somente se percentual/tamanho mudar. Nada de animação/polling de alta frequência.
- I/O fora da thread do menu, timeout de 750 ms por transferência, no máximo duas tentativas, cancelamento drenado antes de liberar buffers.
- Validar report ID, comprimento, status, transação, comando e checksum; erros não devem encerrar o app.
- Instância única por sessão, limpeza de recursos GDI/HID, recuperação do ícone após recriação da bandeja pelo Explorer, DPI por monitor.
- Installer Inno Setup por usuário, sem elevação, em `%LOCALAPPDATA%\Programs\Basilisk Battery`. Fechar instância do app antes de atualizar/desinstalar.

## Entrega

- Repositório Git/GitHub com código, documentação, `VERSION`, changelog, `ERRORS.md` e workflow Windows.
- Cada versão entregue tem release no GitHub, instalador, portátil e checksum; notas concisas para usuário final. Não apagar releases anteriores.
- Validar localmente e no GitHub. Registrar o que foi realmente testado e diferenciar suporte implementado de conexão fisicamente validada.
