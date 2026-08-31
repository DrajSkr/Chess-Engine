import re

with open("frontend/src/App.jsx", "r", encoding="utf-8") as f:
    content = f.read()

# 1. Add PIECE_THEMES
piece_themes_const = """
const PIECE_THEMES = {
  cburnett: { name: 'Classic' },
  merida: { name: 'Merida' },
  alpha: { name: 'Alpha' },
  fantasy: { name: 'Fantasy' }
};

const PIECES_LIST = ['wK', 'wQ', 'wR', 'wB', 'wN', 'wP', 'bK', 'bQ', 'bR', 'bB', 'bN', 'bP'];
"""
content = re.sub(
    r"(const THEMES = \{[\s\S]*?\};\n)",
    r"\1" + piece_themes_const,
    content
)

# 2. Add pieceTheme state
state_add = """  const [pieceTheme, setPieceTheme] = useState('cburnett');"""
content = re.sub(
    r"(const \[boardTheme, setBoardTheme\] = useState\('midnight'\);)",
    r"\1\n" + state_add,
    content
)

# 3. Add customPieces useMemo hook
custom_pieces_hook = """  const customPieces = useMemo(() => {
    const pieceComponents = {};
    PIECES_LIST.forEach(p => {
      pieceComponents[p] = ({ squareWidth }) => (
        <div style={{
          width: squareWidth,
          height: squareWidth,
          backgroundImage: `url(/pieces/${pieceTheme}/${p}.svg)`,
          backgroundSize: '100%',
          backgroundRepeat: 'no-repeat',
          backgroundPosition: 'center'
        }} />
      );
    });
    return pieceComponents;
  }, [pieceTheme]);"""
content = re.sub(
    r"(const finalSquareStyles = useMemo\(\(\) => \{)",
    custom_pieces_hook + r"\n\n  \1",
    content
)

# 4. Add customPieces prop to Chessboard
content = re.sub(
    r"(customLightSquareStyle=\{\{ backgroundColor: THEMES\[boardTheme\]\.light \}\})",
    r"\1\n              customPieces={customPieces}",
    content
)

# 5. Add piece theme selector to controls
piece_theme_selector = """          <select 
            className="btn btn-secondary" 
            value={pieceTheme} 
            onChange={(e) => setPieceTheme(e.target.value)}
            title="Change Piece Theme"
          >
            {Object.entries(PIECE_THEMES).map(([key, theme]) => (
              <option key={key} value={key}>{theme.name} Pieces</option>
            ))}
          </select>"""
content = re.sub(
    r"(<select \n\s*className=\"btn btn-secondary\" \n\s*value=\{boardTheme\})",
    piece_theme_selector + r"\n          \1",
    content
)

with open("frontend/src/App.jsx", "w", encoding="utf-8") as f:
    f.write(content)
