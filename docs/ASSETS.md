# Identidade visual

- Ferramenta: gerador de imagens integrado do ChatGPT (`image_gen`), transparência solicitada e canal alfa RGBA confirmado.
- Fonte: `assets/logo.png`, 1254 × 1254, fundo transparente.
- Ícone do executável/instalador: `assets/app.ico`, derivado da logo com tamanhos 16, 24, 32, 48, 64, 128 e 256 px. A conversão mantém o canal alfa e não adiciona fundo.
- Ícones dinâmicos: GDI nativo em `src/icon.cpp`, bateria horizontal, percentuais e check, desenhados em resolução 4× e reduzidos com alfa premultiplicado. Não dependem da imagem gerada para a leitura numérica.

## Prompt usado

> Use case: logo-brand. Create a minimalist Windows utility app icon for 'Basilisk Battery', a battery monitor for a wireless gaming mouse. One simple recognizable mark: rounded rectangular battery silhouette, with a tiny terminal at top, subtly combined with the clean outline of a computer mouse and a small charging lightning bolt. Flat vector-like solid Razer green #44D62C with a small charcoal #141414 internal accent if helpful. Bold geometric strokes, highly legible at 32 pixels. Centered isolated symbol, no letters, no words, no percentages, no brand snake emblem, no shadow, no glow, no gradients, no 3D, no mockup. Transparent background with real alpha. Square composition with restrained padding. Finished cohesive professional minimal icon, no decorative additions.
