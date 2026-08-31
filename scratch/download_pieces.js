const fs = require('fs');
const path = require('path');

const themes = ['cburnett', 'merida', 'alpha', 'fantasy'];
const pieces = ['wK', 'wQ', 'wR', 'wB', 'wN', 'wP', 'bK', 'bQ', 'bR', 'bB', 'bN', 'bP'];
const baseUrl = 'https://raw.githubusercontent.com/lichess-org/lila/master/public/piece/';

async function downloadPieces() {
  const promises = [];
  for (const t of themes) {
    const dir = path.join(__dirname, 'frontend', 'public', 'pieces', t);
    fs.mkdirSync(dir, { recursive: true });
    
    for (const p of pieces) {
      const url = `${baseUrl}${t}/${p}.svg`;
      const filePath = path.join(dir, `${p}.svg`);
      
      promises.push(
        fetch(url)
          .then(res => {
            if (!res.ok) throw new Error(`Status ${res.status}`);
            return res.text();
          })
          .then(text => fs.promises.writeFile(filePath, text))
          .then(() => console.log(`Downloaded ${t}/${p}.svg`))
          .catch(err => console.error(`Failed ${t}/${p}.svg:`, err.message))
      );
    }
  }
  
  await Promise.all(promises);
  console.log('All done!');
}

downloadPieces();
