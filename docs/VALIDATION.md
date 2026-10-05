# Validação das versões 1.0.0 e 1.0.1

Evidência local registrada em 2026-10-05. Resultados pertencem à máquina e à amostra indicadas; não equivalem a garantia universal de desempenho.

## Testes realizados

- Build nativo x64 com w64devkit 2.10.0, warnings tratados como erros; executável de 111.104 bytes, somente imports de DLLs do Windows.
- Instalador Inno Setup 6.7.3 de aproximadamente 2,2 MB, instalação silenciosa por usuário bem-sucedida. Executável instalado e `DisplayVersion` no uninstall: `1.0.0`.
- Instância única: segunda execução retorna sem criar outro processo residente.
- Testes de protocolo: framing/report ID, comprimento 90/91, checksum, transação, colisão bateria/carregamento, ausência de resposta, conversão monotônica e limites 25/50/100.
- Testes de integração da inicialização: ativar, verificar comando com caminho entre aspas, desativar e repetir desativação; valor original do registro preservado/restaurado.
- Diagnóstico físico: Basilisk V3 Pro 35K, receptor `1532:00CD`, 100%, charging=0, sem erro Windows.
- 55 ícones exportados em 16/20/24/32/48 px; inspeção em fundos claro e escuro, limiares, `88`/`99`, desconhecido e check. Tipografia ajustada e aprovada por consultor `gpt-6-astra`, esforço `medium`.
- Runtime instalado: amostra de 161,953 segundos, consultas periódicas ativas; nenhum incremento mensurável no contador de CPU, working set 15,1 MiB, memória privada 3,39 MiB, 186 handles sem crescimento. A resolução do contador limita a precisão; não afirmar CPU sempre zero.

## Limites da evidência

- Automação visual do menu foi interrompida por Escape físico do usuário; não foi retomada. As funções de inicialização foram testadas diretamente, mas o clique e o check do menu ainda requerem validação visual.
- Cabo USB, V3 Pro original, troca física de conexão, suspensão/retomada, recriação do Explorer e reboot real não foram exercitados fisicamente nesta sessão. Os handlers/suporte existem; não chamar isso de prova física.
- O workflow usa MSVC x64 no GitHub e deve concluir antes de considerar a release pública entregue. Os arquivos produzidos por MSVC podem ter tamanho diferente do build local MinGW; avaliar os arquivos efetivamente publicados.
- O projeto não possui certificado de assinatura de código configurado.

## Repetir diagnóstico

```powershell
$app = "$env:LOCALAPPDATA\Programs\Basilisk Battery\BasiliskBattery.exe"
Start-Process -FilePath $app -ArgumentList '--diagnose', 'battery.json' -Wait
Get-Content battery.json
```

Os relatórios locais e imagens de verificação ficam em `artifacts/` (ignorado no Git). O script de build também verifica a versão e executa os testes no CI.

## Ajuste 1.0.1

Por solicitação do usuário, peso da fonte aumentado de Semibold (600) para Bold (700), preservando altura 10/16 e padding. Os testes de protocolo e integração Windows continuam passando; exportação dos 55 ícones repetida para verificar o peso final. Esse ajuste não altera o transporte HID nem a frequência de consultas.
