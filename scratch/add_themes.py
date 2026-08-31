import re

with open("frontend/src/App.jsx", "r", encoding="utf-8") as f:
    content = f.read()

# 1. Add THEMES constant
themes_const = """
const THEMES = {
  emerald: { name: 'Emerald', light: '#edeed1', dark: '#779952' },
  ocean: { name: 'Ocean', light: '#dee3e6', dark: '#8ca2ad' },
  midnight: { name: 'Midnight', light: '#d1d9e1', dark: '#5c7080' },
  walnut: { name: 'Walnut', light: '#e4c49d', dark: '#8b5a2b' },
  coral: { name: 'Coral', light: '#f2e3d5', dark: '#d88373' },
  cyber: { name: 'Cyber', light: '#e2e8f0', dark: '#8b5cf6' }
};

"""
content = re.sub(
    r"(const MODES = \{)",
    themes_const + r"\1",
    content
)

# 2. Add boardTheme state
state_add = """  const [boardTheme, setBoardTheme] = useState('midnight');"""
content = re.sub(
    r"(const \[boardOrientation, setBoardOrientation\] = useState\('white'\);)",
    r"\1\n" + state_add,
    content
)

# 3. Update Chessboard component styles
content = re.sub(
    r"customDarkSquareStyle=\{\{ backgroundColor: '#779952' \}\}",
    r"customDarkSquareStyle={{ backgroundColor: THEMES[boardTheme].dark }}",
    content
)
content = re.sub(
    r"customLightSquareStyle=\{\{ backgroundColor: '#edeed1' \}\}",
    r"customLightSquareStyle={{ backgroundColor: THEMES[boardTheme].light }}",
    content
)

# 4. Add theme selector to controls
theme_selector = """          <select 
            className="btn btn-secondary" 
            value={boardTheme} 
            onChange={(e) => setBoardTheme(e.target.value)}
            title="Change Board Theme"
          >
            {Object.entries(THEMES).map(([key, theme]) => (
              <option key={key} value={key}>{theme.name}</option>
            ))}
          </select>"""
content = re.sub(
    r"(<button className=\"btn btn-secondary\" onClick=\{handleFlipBoard\}>\s*🔃 Flip Board\s*</button>)",
    r"\1\n" + theme_selector,
    content
)

# 5. Add sound to jumpToMove
jump_to_move_sound = """    if (index === -1) {
      setGame(new Chess(initialFen));
    } else {
      setGame(new Chess(moveHistory[index].fen));
      playSoundForMove(moveHistory[index].san, false);
    }"""
content = re.sub(
    r"if \(index === -1\) \{\s*setGame\(new Chess\(initialFen\)\);\s*\} else \{\s*setGame\(new Chess\(moveHistory\[index\]\.fen\)\);\s*\}",
    jump_to_move_sound,
    content
)

with open("frontend/src/App.jsx", "w", encoding="utf-8") as f:
    f.write(content)
