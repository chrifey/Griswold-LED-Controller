

int getPaletteCount() {
  Dir dir = SPIFFS.openDir("/palettes");
  int palette_count = 0;
  while (dir.next()) {
    palette_count++;
  }
  DBG_OUTPUT_PORT.printf("Palette count: %d\n", palette_count);
  return palette_count;
}

bool openPaletteFileWithIndex(int index, File* file) {
  Dir dir = SPIFFS.openDir("/palettes");

  int palette_count = 0;
  while (dir.next()) {
    if (palette_count == index) break;
    palette_count++;
  }
  
  if (palette_count != index) {
    DBG_OUTPUT_PORT.println("Error, unable to open palette");
    return false;
  }
  
  *file = dir.openFile("r");
  return true; //success
}

bool loadPaletteFromFile(int index, CRGBPalette16* palette) {
  File paletteFile;
  if (!openPaletteFileWithIndex(index, &paletteFile)) {
      DBG_OUTPUT_PORT.printf("Error loading paletteFile at index %d\n", index);
      return false;
  }
  
  int paletteFileSize = paletteFile.size();
  uint8_t* bytes = new uint8_t[paletteFileSize];
  if (!bytes) {
    DBG_OUTPUT_PORT.println("Unable to allocate memory for palette");
    return false;
  }

  paletteFile.readBytes((char*)bytes, paletteFileSize);
  paletteFile.close();
  
  DBG_OUTPUT_PORT.printf("Load palette named %s (%d bytes)\n", paletteFile.name(), paletteFileSize);

  palette->loadDynamicGradientPalette(bytes);

  delete[] bytes;
  return true;
}


String getPaletteNameWithIndex(int index) {
  Dir dir = SPIFFS.openDir("/palettes");

  int ndx = 0;
  while (dir.next()) {
    if (ndx == index) return dir.fileName();
    ndx++;
  }
  return "[unknown]";
}
