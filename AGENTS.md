# Orientações para agentes

## Produto e escopo

Leia [docs/SPEC.md](docs/SPEC.md) antes de alterar o comportamento. Basilisk Battery é um app nativo C++/Win32, Windows 10/11 x64, exclusivamente na área de notificações. Consumo de CPU, tamanho do instalador e estabilidade são prioridades. Não adicionar frameworks, runtime, serviço, telemetria, rede ou dependências residentes sem necessidade aprovada.

## Versionamento obrigatório

- `VERSION` é a fonte única da versão do produto, SemVer estável `MAJOR.MINOR.PATCH`, sem `v`.
- Toda alteração entregue deve atualizar `VERSION` e a seção correspondente de `CHANGELOG.md`: patch para correção, minor para recurso compatível, major para quebra.
- A versão do executável, do menu e do instalador vem automaticamente de `VERSION` via `scripts/build.ps1`. Não duplicar a versão em código. O assemblyIdentity do manifesto identifica o aplicativo Windows, não a versão do produto.
- Notas de release em português, breves e voltadas ao usuário; evitar detalhes internos de código/compilador.
- O workflow compila e testa em PR/push. Em `main`, publica automaticamente a tag `vX.Y.Z`, instalador, executável portátil e SHA-256 se essa release ainda não existe. Preservar releases antigas; nunca sobrescrever arquivos de uma versão publicada.
- Não declarar release concluída sem conferir sucesso do workflow, tag/commit e arquivos realmente disponíveis no GitHub.

## Validação e entrega

- Trabalhar diretamente; não usar subagentes ou gates de orquestração sem pedido explícito do usuário.
- Consultar `ERRORS.md` antes de investigar erro relevante; registrar sintomas, causa, solução, evidência e prevenção para erros não triviais resolvidos.
- Executar `scripts/build.ps1 -Installer` em Developer PowerShell x64 do Visual Studio. Alternativa portátil: `-Toolchain <pasta bin do w64devkit>`.
- Validar protocolo, limiares 25/50/100, ícones em 16/20/24/32/48 px, menu, inicialização ativada/desativada e instância única.
- Diagnóstico físico: `BasiliskBattery.exe --diagnose <arquivo.json>`. Código 0 indica leitura válida; 1 indica indisponível; 2 indica falha de execução. Nunca confundir diagnóstico sintético com prova do hardware.
- Medir CPU/memória em runtime com consultas ativas; não afirmar consumo zero com base apenas no código.
- Instalar o setup produzido e validar caminho/versão do processo instalado e `DisplayVersion` no registro. Preferências existentes devem sobreviver à atualização.
- Testar upgrade com app aberto (`scripts/test-installer.ps1`, em ambiente de teste com startup do produto ausente). Nunca usar `New-Item -Force` em chave compartilhada do registro; criar somente se ausente e verificar preservação das demais entradas.
- No resumo, informar `Versão esperada: X.Y.Z`, link da release, evidências e limitações físicas pendentes.
- Preservar alterações não relacionadas. Stage apenas caminhos nomeados; não usar `git add -A`.
