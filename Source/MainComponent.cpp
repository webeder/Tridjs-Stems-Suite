#include "MainComponent.h"
#include "StemEngine.h"

// Define styling colors based on HTML neon theme
const juce::Colour colBackground = juce::Colour::fromString("#FF131313");
const juce::Colour colSurface = juce::Colour::fromString("#FF2A2A2A");
const juce::Colour colPrimary = juce::Colour::fromString("#FFADC7FF");
const juce::Colour colVocals = colPrimary;
const juce::Colour colDrums =
    juce::Colour::fromString("#FF50FFAF");                          // Secondary
const juce::Colour colBass = juce::Colour::fromString("#FFECC31B"); // Tertiary
const juce::Colour colOther = juce::Colour::fromString("#FF8B90A0"); // Outline

//==============================================================================
MainComponent::MainComponent() {
  setLookAndFeel(&customLookAndFeel);
  setSize(800, 600);

  // Load Splash Screen Image
  juce::File splashFile("C:\\StemsTriDJs\\splash.png");
  if (!splashFile.existsAsFile())
    splashFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                     .getSiblingFile("resources")
                     .getChildFile("splash.png");
  if (!splashFile.existsAsFile())
    splashFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                     .getSiblingFile("splash.png");

  if (splashFile.existsAsFile())
    splashImage = juce::ImageFileFormat::loadFrom(splashFile);

  // Initial Screen Setup
  addAndMakeVisible(selectFileBtn);
  selectFileBtn.setColour(juce::TextButton::buttonColourId,
                          colPrimary.withAlpha(0.2f));
  selectFileBtn.setColour(juce::TextButton::textColourOffId, colPrimary);
  selectFileBtn.onClick = [this] { openFileChooser(); };

  // Processing Screen Setup
  addChildComponent(progressBar);
  progressBar.setColour(juce::ProgressBar::foregroundColourId, colDrums);
  progressBar.setColour(juce::ProgressBar::backgroundColourId, colSurface);

  addChildComponent(progressLabel);
  progressLabel.setFont(juce::Font("Geist", 28.0f, juce::Font::bold));
  progressLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  progressLabel.setJustificationType(juce::Justification::centred);
  progressLabel.setText("Separando Stems...", juce::dontSendNotification);

  addChildComponent(cancelBtn);
  cancelBtn.onClick = [this] {
    if (engineWorker != nullptr) {
      engineWorker->signalThreadShouldExit();
      engineWorker->waitForThreadToExit(2000);
      engineWorker.reset();
    }
    currentState = AppState::ScreenUpload;
    resized();
    repaint();
  };
  cancelBtn.setVisible(false);

  // Results Screen Setup
  addAndMakeVisible(downloadAllBtn);
  downloadAllBtn.setColour(juce::TextButton::buttonColourId,
                           colPrimary.withAlpha(0.2f));
  downloadAllBtn.setColour(juce::TextButton::textColourOffId, colPrimary);
  downloadAllBtn.onClick = [this] { exportAllAsZip(); };
  downloadAllBtn.setVisible(false);

  addChildComponent(clearBtn);
  clearBtn.onClick = [this] {
    // Stop audio playback immediately before clearing readers
    sourcePlayer.setSource(nullptr);
    mixerSource.removeAllInputs();

    // Hide and clear all stem players
    for (auto &player : stemPlayers) {
      player->playBtn.setVisible(false);
      player->stopBtn.setVisible(false);
      player->downloadBtn.setVisible(false);
      player->seekBar.setVisible(false);
    }
    stemPlayers.clear();

    cleanTempDirectory();
    currentState = AppState::ScreenUpload;
    resized();
    repaint();
  };

  // Audio Engine Setup
  formatManager.registerBasicFormats();
  deviceManager.initialiseWithDefaultDevices(0, 2);
  deviceManager.addAudioCallback(&sourcePlayer);
  sourcePlayer.setSource(&mixerSource);

  setupTempDirectory();

  // Inicia o Timer para atualizar animações da interface gráfica
  startTimer(30);
}

void MainComponent::startWarmup() {
  // Inicializa o preload e warmup da engine LibTorch/CUDA nativa em background
  engineWorker = std::make_unique<StemEngineWorker>(
      StemEngineWorker::Job::Warmup, [this]() {
        bool success = (StemEngine::getInstance().getStatus() == StemEngine::Status::Ready);
        
        // Transiciona a interface gráfica para a tela de upload principal!
        currentState = AppState::ScreenUpload;
        resized();
        repaint();

        if (onInitializationComplete != nullptr) {
          onInitializationComplete(success);
        }
      });
  engineWorker->startThread();
}

MainComponent::~MainComponent() {
  setLookAndFeel(nullptr);
  sourcePlayer.setSource(nullptr);
  deviceManager.removeAudioCallback(&sourcePlayer);

  stopTimer();

  // Garante que o worker em segundo plano seja parado
  if (engineWorker != nullptr) {
    engineWorker->signalThreadShouldExit();
    engineWorker->waitForThreadToExit(2000);
    engineWorker.reset();
  }

  // Desliga os recursos da LibTorch na memória
  #include "StemEngine.h"
  StemEngine::getInstance().shutdown();

  cleanTempDirectory();
}

void MainComponent::setupTempDirectory() {
  tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                .getChildFile("TriDJs_Stems_Cache");
  cleanTempDirectory(); // Clean any previous leftover
  tempDir.createDirectory();
}

void MainComponent::cleanTempDirectory() {
  if (tempDir.exists())
    tempDir.deleteRecursively();
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(colBackground);

  if (currentState == AppState::ScreenSplash) {
    if (splashImage.isValid()) {
      auto area = getLocalBounds().toFloat();
      auto splashArea = juce::Rectangle<float>(500.0f, 500.0f).withCentre(area.getCentre());
      g.drawImage(splashImage, splashArea, juce::RectanglePlacement::stretchToFit);
    } else {
      g.setColour(juce::Colours::white);
      g.setFont(juce::Font("Geist", 20.0f, juce::Font::bold));
      g.drawText("Carregando engine de IA...", getLocalBounds(), juce::Justification::centred);
    }
    return;
  }

  // Top Header
  g.setColour(colPrimary);
  g.setFont(juce::Font(24.0f, juce::Font::bold));
  g.drawText("TriDJs Stems", 24, 16, 200, 40, juce::Justification::centredLeft);

  // Separator line
  g.setColour(colSurface);
  g.drawLine(0.0f, 70.0f, (float)getWidth(), 70.0f, 1.0f);

  switch (currentState) {
  case AppState::ScreenUpload:
    drawUploadScreen(g);
    break;
  case AppState::ScreenProcessing:
    drawProcessingScreen(g);
    break;
  case AppState::ScreenResults:
    drawResultsScreen(g);
    break;
  }
}

void MainComponent::drawUploadScreen(juce::Graphics &g) {
  auto area = getLocalBounds().reduced(24);

  // Top App Bar has 60px height
  area.removeFromTop(60);

  // 1. Draw Title Area
  auto titleArea = area.removeFromTop(90);
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Geist", 28.0f, juce::Font::bold));
  g.drawText("Separe Suas Faixas", titleArea.removeFromTop(40),
             juce::Justification::centred);

  g.setColour(juce::Colour::fromString("#FFC1C6D7")); // on-surface-variant
  g.setFont(juce::Font("Geist", 14.0f, juce::Font::plain));
  g.drawText("Envie qualquer arquivo de áudio para gerar stems de nível "
             "profissional (Bateria, Baixo, Vocal, Outros).",
             titleArea, juce::Justification::centred);

  // 2. Main Bento Grid Area (remaining space minus bottom section)
  auto bentoArea = area.removeFromTop(area.getHeight() - 110);

  // 12 columns total: left is 7 cols, right is 3 cols
  auto leftBentoWidth = (bentoArea.getWidth() * 7) / 10;

  auto leftCard = bentoArea.removeFromLeft(leftBentoWidth).reduced(8);
  auto rightColumn = bentoArea.reduced(8);

  // A) Draw Left Upload Zone
  g.setColour(juce::Colour::fromString("#FF201F1F")); // surface-container
  g.fillRoundedRectangle(leftCard.toFloat(), 12.0f);

  // Draw Dashed Border
  g.setColour(colPrimary.withAlpha(0.3f));
  g.drawRoundedRectangle(leftCard.toFloat().reduced(2.0f), 12.0f, 2.0f);

  // Draw Upload Icon (large arrow pointing up inside circle)
  auto iconArea =
      leftCard.withSizeKeepingCentre(60, 60).withY(leftCard.getCentreY() - 80);
  g.setColour(colPrimary.withAlpha(0.1f));
  g.fillEllipse(iconArea.toFloat());
  g.setColour(colPrimary);
  g.drawEllipse(iconArea.toFloat(), 2.0f);

  // Arrow up graphic
  g.drawLine((float)iconArea.getCentreX(), (float)iconArea.getY() + 15.0f,
             (float)iconArea.getCentreX(), (float)iconArea.getBottom() - 15.0f,
             3.0f);
  g.drawLine((float)iconArea.getCentreX() - 8.0f,
             (float)iconArea.getY() + 23.0f, (float)iconArea.getCentreX(),
             (float)iconArea.getY() + 15.0f, 3.0f);
  g.drawLine((float)iconArea.getCentreX() + 8.0f,
             (float)iconArea.getY() + 23.0f, (float)iconArea.getCentreX(),
             (float)iconArea.getY() + 15.0f, 3.0f);

  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Geist", 20.0f, juce::Font::bold));
  g.drawText("Envie ou arraste sua música aqui",
             leftCard.withY(leftCard.getCentreY() - 10).withHeight(30),
             juce::Justification::centred);

  g.setColour(juce::Colour::fromString("#FF8B90A0"));
  g.setFont(juce::Font("JetBrains Mono", 11.0f, juce::Font::plain));
  g.drawText("MP3, WAV ou FLAC até 50MB",
             leftCard.withY(leftCard.getCentreY() + 20).withHeight(20),
             juce::Justification::centred);

  // B) Draw Right Bento Cards (Stacked - 3 Cards)
  int cardHeight = (rightColumn.getHeight() - 24) / 3;
  auto card1 = rightColumn.removeFromTop(cardHeight);
  rightColumn.removeFromTop(12); // gap
  auto card2 = rightColumn.removeFromTop(cardHeight);
  rightColumn.removeFromTop(12); // gap
  auto card3 = rightColumn;

  // Card 1: Lossless Output (Saída Sem Perdas)
  g.setColour(juce::Colour::fromString("#FF1C1B1B")); // surface-container-low
  g.fillRoundedRectangle(card1.toFloat(), 12.0f);
  g.setColour(juce::Colour::fromString("#FF353534")); // border
  g.drawRoundedRectangle(card1.toFloat(), 12.0f, 1.0f);

  auto card1Text = card1.reduced(16);
  auto hqIconArea = card1Text.removeFromTop(20);
  auto hqSquare = hqIconArea.removeFromLeft(28).withSizeKeepingCentre(24, 16);
  g.setColour(colDrums); // neon green
  g.fillRoundedRectangle(hqSquare.toFloat(), 3.0f);
  g.setColour(juce::Colour::fromString("#FF002111"));
  g.setFont(juce::Font("JetBrains Mono", 10.0f, juce::Font::bold));
  g.drawText("HQ", hqSquare, juce::Justification::centred);

  g.setColour(colDrums);
  g.setFont(juce::Font("Geist", 15.0f, juce::Font::bold));
  g.drawText("Saída Sem Perdas", hqIconArea.translated(8, 0),
             juce::Justification::centredLeft);

  g.setColour(juce::Colour::fromString("#FFC1C6D7"));
  g.setFont(juce::Font("Geist", 12.0f, juce::Font::plain));
  g.drawText(
      "Nossa IA mantém a máxima fidelidade de 24 bits durante todo o processo.",
      card1Text, juce::Justification::topLeft, true);

  // Card 2: Ultra Fast (Ultra Rápido)
  g.setColour(juce::Colour::fromString("#FF1C1B1B"));
  g.fillRoundedRectangle(card2.toFloat(), 12.0f);
  g.setColour(juce::Colour::fromString("#FF353534"));
  g.drawRoundedRectangle(card2.toFloat(), 12.0f, 1.0f);

  auto card2Text = card2.reduced(16);
  auto boltIconArea = card2Text.removeFromTop(20);
  auto boltSquare = boltIconArea.removeFromLeft(20);
  g.setColour(colBass); // neon yellow
  juce::Path boltPath;
  boltPath.startNewSubPath(boltSquare.getX() + 12.0f, boltSquare.getY() + 1.0f);
  boltPath.lineTo(boltSquare.getX() + 5.0f, boltSquare.getY() + 10.0f);
  boltPath.lineTo(boltSquare.getX() + 10.0f, boltSquare.getY() + 10.0f);
  boltPath.lineTo(boltSquare.getX() + 8.0f, boltSquare.getY() + 18.0f);
  boltPath.lineTo(boltSquare.getX() + 15.0f, boltSquare.getY() + 9.0f);
  boltPath.lineTo(boltSquare.getX() + 10.0f, boltSquare.getY() + 9.0f);
  boltPath.closeSubPath();
  g.fillPath(boltPath);

  g.setColour(colBass);
  g.setFont(juce::Font("Geist", 15.0f, juce::Font::bold));
  g.drawText("Ultra Rápido", boltIconArea.translated(8, 0),
             juce::Justification::centredLeft);

  g.setColour(juce::Colour::fromString("#FFC1C6D7"));
  g.setFont(juce::Font("Geist", 12.0f, juce::Font::plain));
  g.drawText("Tempo estimado: ~45 segundos para faixas de duração média.",
             card2Text, juce::Justification::topLeft, true);

  // Card 3: Recent Stems (Stems Recentes)
  g.setColour(juce::Colour::fromString("#FF1C1B1B"));
  g.fillRoundedRectangle(card3.toFloat(), 12.0f);
  g.setColour(juce::Colour::fromString("#FF353534"));
  g.drawRoundedRectangle(card3.toFloat(), 12.0f, 1.0f);

  auto card3Text = card3.reduced(16);
  auto recentHeader = card3Text.removeFromTop(20);

  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Geist", 13.0f, juce::Font::bold));
  g.drawText("Stems Recentes", recentHeader, juce::Justification::centredLeft);

  // Draw "..." menu on the right
  g.setColour(juce::Colour::fromString("#FF8B90A0"));
  g.setFont(juce::Font("Geist", 14.0f, juce::Font::bold));
  g.drawText("...", recentHeader, juce::Justification::centredRight);

  card3Text.removeFromTop(8); // gap

  // Draw file preview box
  auto fileBox = card3Text.removeFromTop(40);

  // Draw a tiny vinyl disc preview
  auto discArea = fileBox.removeFromLeft(32).withSizeKeepingCentre(24, 24);
  g.setColour(juce::Colour::fromString("#FF2A2A2A"));
  g.fillEllipse(discArea.toFloat());
  g.setColour(colVocals.withAlpha(0.6f));
  g.drawEllipse(discArea.toFloat().reduced(2.0f), 1.5f);
  g.setColour(juce::Colours::black);
  g.fillEllipse(discArea.toFloat().reduced(8.0f));

  fileBox.removeFromLeft(8); // gap

  // File name
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("JetBrains Mono", 10.0f, juce::Font::bold));
  g.drawText("Midnight Echo.wav", fileBox.removeFromTop(16),
             juce::Justification::topLeft);

  // Prefilled progress bar (80%)
  auto miniBar = fileBox.reduced(0, 4);
  g.setColour(juce::Colour::fromString("#FF201F1F"));
  g.fillRoundedRectangle(miniBar.toFloat(), 3.0f);

  auto miniBarFill = miniBar.withWidth((int)(miniBar.getWidth() * 0.8f));
  g.setColour(colDrums);
  g.fillRoundedRectangle(miniBarFill.toFloat(), 3.0f);

  // 3. Draw Bottom Section: AI Extraction Architecture (Arquitetura de Extração
  // por IA)
  auto bottomArea = area;
  g.setColour(juce::Colour::fromString("#FF8B90A0"));
  g.setFont(juce::Font("JetBrains Mono", 11.0f, juce::Font::bold));
  g.drawText("ARQUITETURA DE EXTRAÇÃO POR IA", bottomArea.removeFromTop(20),
             juce::Justification::centredLeft);

  auto stemGrid = bottomArea.reduced(2);
  int stemBoxWidth = (stemGrid.getWidth() - 24) / 4;

  juce::Colour stemCols[] = {colVocals, colDrums, colBass, colOther};
  juce::String stemNames[] = {"Vocais", "Bateria", "Baixo", "Outros"};

  for (int i = 0; i < 4; ++i) {
    auto box = stemGrid.removeFromLeft(stemBoxWidth);
    stemGrid.removeFromLeft(8); // gap

    g.setColour(juce::Colour::fromString("#FF201F1F"));
    g.fillRoundedRectangle(box.toFloat(), 8.0f);

    // Colored left accent line
    g.setColour(stemCols[i]);
    g.fillRoundedRectangle((float)box.getX(), (float)box.getY(), 4.0f,
                           (float)box.getHeight(), 2.0f);

    auto boxText = box.reduced(8);
    boxText.removeFromLeft(4); // account for accent line

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font("Geist", 13.0f, juce::Font::bold));
    g.drawText(stemNames[i], boxText.removeFromTop(16),
               juce::Justification::centredLeft);

    // Draw decorative preview waveform (fully colored in on screen 1)
    g.setColour(stemCols[i].withAlpha(0.6f));
    int barCount = 10;
    int barWidth = (boxText.getWidth() - (barCount - 1) * 2) / barCount;
    if (barWidth < 1)
      barWidth = 1;
    int heights[] = {8, 14, 22, 16, 26, 12, 18, 24, 10, 6};
    for (int b = 0; b < barCount; ++b) {
      int h = heights[(b + i * 2) % 10] * boxText.getHeight() / 32;
      g.fillRoundedRectangle((float)(boxText.getX() + b * (barWidth + 2)),
                             (float)(boxText.getCentreY() - h / 2 + 4),
                             (float)barWidth, (float)h, 1.0f);
    }
  }
}

void MainComponent::drawProcessingScreen(juce::Graphics &g) {
  auto area = getLocalBounds().reduced(24);
  area.removeFromTop(60);

  // 1. Titles (Handled dynamically by progressLabel now, but we still advance
  // layout area)
  area.removeFromTop(40);

  g.setColour(juce::Colour::fromString("#FF8B90A0"));
  g.setFont(juce::Font("JetBrains Mono", 11.0f, juce::Font::bold));
  g.drawText("NEURAL ENGINE ANALYSIS IN PROGRESS", area.removeFromTop(20),
             juce::Justification::centred);

  area.removeFromTop(20); // gap

  // 2. Bento Progress Card
  auto card = area.withSizeKeepingCentre(480, 260);
  g.setColour(juce::Colour::fromString("#FF1C1B1B")); // surface-container-low
  g.fillRoundedRectangle(card.toFloat(), 16.0f);
  g.setColour(juce::Colour::fromString("#FF353534")); // border
  g.drawRoundedRectangle(card.toFloat(), 16.0f, 1.5f);

  auto cardContent = card.reduced(24);

  // Top row: STATUS on left, % on right
  auto topRow = cardContent.removeFromTop(50);
  auto statusArea = topRow.removeFromLeft(200);

  g.setColour(colDrums); // neon green
  g.setFont(juce::Font("JetBrains Mono", 10.0f, juce::Font::bold));
  g.drawText("STATUS", statusArea.removeFromTop(14),
             juce::Justification::bottomLeft);
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Geist", 18.0f, juce::Font::bold));
  g.drawText("Extraction Active", statusArea, juce::Justification::topLeft);

  // Percentage
  g.setColour(colDrums);
  g.setFont(juce::Font("Geist", 36.0f, juce::Font::bold));

  // Read parsed percentage directly from persistent progressValue (normalized
  // 0.0 - 1.0)
  int percent = (int)std::round(progressValue * 100.0);
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;

  double fillRatio = progressValue;
  if (fillRatio < 0.05)
    fillRatio = 0.05; // minimum fill

  juce::String percentStr = percent > 0 ? juce::String(percent) + "%" : "0%";
  g.drawText(percentStr, topRow, juce::Justification::centredRight);

  cardContent.removeFromTop(10); // gap
 
  // Progress bar area layout advancement (drawn by native progressBar component)
  auto barArea = cardContent.removeFromTop(16);
 
  cardContent.removeFromTop(20); // gap

  // Draw 4 mini stem items at bottom of bento card
  auto grid = cardContent;
  int colWidth = (grid.getWidth() - 18) / 4;

  juce::Colour stemColors[] = {colVocals, colDrums, colBass, colOther};
  juce::String names[] = {"Vocals", "Drums", "Bass", "Inst."};

  for (int i = 0; i < 4; ++i) {
    auto item = grid.removeFromLeft(colWidth);
    grid.removeFromLeft(6); // gap

    g.setColour(juce::Colour::fromString("#FF201F1F")); // surface-container
    g.fillRoundedRectangle(item.toFloat(), 8.0f);

    g.setColour(stemColors[i]);
    g.fillRoundedRectangle((float)item.getX(), (float)item.getY(), 3.0f,
                           (float)item.getHeight(), 1.5f); // colored left bar

    auto itemText = item.reduced(6);
    itemText.removeFromLeft(3);

    // Draw premium vector outline icons matching FontAwesome / Material Symbols
    auto iconArea = itemText.removeFromTop(18).toFloat();
    g.setColour(stemColors[i]);

    if (i == 0) // Mic Icon (Vocals)
    {
      g.drawRoundedRectangle(iconArea.getX() + 4.0f, iconArea.getY(), 6.0f,
                             10.0f, 3.0f, 1.2f);
      g.drawLine(iconArea.getX() + 4.0f, iconArea.getY() + 4.0f,
                 iconArea.getX() + 10.0f, iconArea.getY() + 4.0f, 1.0f);

      juce::Path stand;
      stand.startNewSubPath(iconArea.getX() + 2.0f, iconArea.getY() + 6.0f);
      stand.lineTo(iconArea.getX() + 2.0f, iconArea.getY() + 10.0f);
      stand.lineTo(iconArea.getX() + 12.0f, iconArea.getY() + 10.0f);
      stand.lineTo(iconArea.getX() + 12.0f, iconArea.getY() + 6.0f);
      g.strokePath(stand, juce::PathStrokeType(1.2f));
      g.drawLine(iconArea.getX() + 7.0f, iconArea.getY() + 10.0f,
                 iconArea.getX() + 7.0f, iconArea.getY() + 14.0f, 1.2f);
    } else if (i == 1) // Layers/Pads Icon (Drums)
    {
      for (int ly = 0; ly < 3; ++ly) {
        float dy = ly * 3.0f;
        juce::Path layerPath;
        layerPath.startNewSubPath(iconArea.getX() + 4.0f, iconArea.getY() + dy);
        layerPath.lineTo(iconArea.getX() + 12.0f, iconArea.getY() + dy);
        layerPath.lineTo(iconArea.getX() + 9.0f, iconArea.getY() + dy + 3.0f);
        layerPath.lineTo(iconArea.getX() + 1.0f, iconArea.getY() + dy + 3.0f);
        layerPath.closeSubPath();
        g.strokePath(layerPath, juce::PathStrokeType(1.0f));
      }
    } else if (i == 2) // Speaker Icon (Bass)
    {
      juce::Path speakerPath;
      speakerPath.startNewSubPath(iconArea.getX() + 1.0f,
                                  iconArea.getY() + 4.0f);
      speakerPath.lineTo(iconArea.getX() + 4.0f, iconArea.getY() + 4.0f);
      speakerPath.lineTo(iconArea.getX() + 8.0f, iconArea.getY() + 1.0f);
      speakerPath.lineTo(iconArea.getX() + 8.0f, iconArea.getY() + 11.0f);
      speakerPath.lineTo(iconArea.getX() + 4.0f, iconArea.getY() + 8.0f);
      speakerPath.lineTo(iconArea.getX() + 1.0f, iconArea.getY() + 8.0f);
      speakerPath.closeSubPath();
      g.fillPath(speakerPath);
      g.drawEllipse(iconArea.getX() + 5.0f, iconArea.getY() + 3.0f, 6.0f, 6.0f,
                    1.0f);
    } else if (i == 3) // Music Note Icon (Inst.)
    {
      g.fillEllipse(iconArea.getX() + 1.0f, iconArea.getY() + 9.0f, 3.5f, 2.5f);
      g.fillEllipse(iconArea.getX() + 8.0f, iconArea.getY() + 7.0f, 3.5f, 2.5f);
      g.drawLine(iconArea.getX() + 4.0f, iconArea.getY() + 2.0f,
                 iconArea.getX() + 4.0f, iconArea.getY() + 9.5f, 1.2f);
      g.drawLine(iconArea.getX() + 11.0f, iconArea.getY() + 1.0f,
                 iconArea.getX() + 11.0f, iconArea.getY() + 7.5f, 1.2f);
      juce::Path beamPath;
      beamPath.startNewSubPath(iconArea.getX() + 4.0f, iconArea.getY() + 2.0f);
      beamPath.lineTo(iconArea.getX() + 11.0f, iconArea.getY() + 1.0f);
      beamPath.lineTo(iconArea.getX() + 11.0f, iconArea.getY() + 3.5f);
      beamPath.lineTo(iconArea.getX() + 4.0f, iconArea.getY() + 4.5f);
      beamPath.closeSubPath();
      g.fillPath(beamPath);
    }

    itemText.removeFromTop(4); // gap after icon

    g.setColour(juce::Colour::fromString("#FF8B90A0"));
    g.setFont(juce::Font("Geist", 11.0f, juce::Font::bold));
    g.drawText(names[i], itemText.removeFromTop(14),
               juce::Justification::centredLeft);

    // Draw tiny colored progress line under each stem depending on progress
    g.setColour(stemColors[i].withAlpha(0.2f));
    g.fillRoundedRectangle((float)itemText.getX(), (float)(itemText.getY() + 4),
                           (float)itemText.getWidth(), 3.0f, 1.5f);

    // Animate them sequentially depending on current percentage
    float stemRatio = 0.0f;
    if (i == 0)
      stemRatio = std::min(1.0f, (float)fillRatio * 2.0f);
    else if (i == 1)
      stemRatio =
          std::max(0.0f, std::min(1.0f, ((float)fillRatio - 0.3f) * 2.0f));
    else if (i == 2)
      stemRatio =
          std::max(0.0f, std::min(1.0f, ((float)fillRatio - 0.6f) * 2.0f));
    else
      stemRatio =
          std::max(0.0f, std::min(1.0f, ((float)fillRatio - 0.8f) * 5.0f));

    g.setColour(stemColors[i]);
    g.fillRoundedRectangle((float)itemText.getX(), (float)(itemText.getY() + 4),
                           (float)itemText.getWidth() * stemRatio, 3.0f, 1.5f);
  }
}

void MainComponent::drawResultsScreen(juce::Graphics &g) {
  auto bounds = getLocalBounds().reduced(24);
  bounds.removeFromTop(60);

  // Draw Title Header for results
  auto headerArea = bounds.removeFromTop(90);
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Geist", 20.0f, juce::Font::bold));
  g.drawText("Stems Extraction Complete", headerArea.removeFromTop(30),
             juce::Justification::centredLeft);

  g.setColour(juce::Colour::fromString("#FF8B90A0"));
  g.setFont(juce::Font("Geist", 13.0f, juce::Font::plain));
  g.drawText("Track: " + inputFile.getFileName(), headerArea,
             juce::Justification::topLeft);

  // Each player box is drawn in resized() bounds, let's replicate the box
  // drawing here
  int yOffset = bounds.getY() + 10;
  int playerHeight = 80;
  int gap = 16;

  for (size_t i = 0; i < stemPlayers.size(); ++i) {
    auto &player = stemPlayers[i];
    auto pBounds = juce::Rectangle<int>(bounds.getX(), yOffset,
                                        bounds.getWidth(), playerHeight);

    // 1. Draw Card Background
    g.setColour(juce::Colour::fromString("#FF1C1B1B")); // surface-container-low
    g.fillRoundedRectangle(pBounds.toFloat(), 12.0f);

    // 2. Draw Left-Border Accent
    g.setColour(player->colour);
    g.fillRoundedRectangle((float)pBounds.getX(), (float)pBounds.getY(), 4.0f,
                           (float)playerHeight, 2.0f);

    // 3. Draw Border outline
    g.setColour(juce::Colour::fromString("#FF353534"));
    g.drawRoundedRectangle(pBounds.toFloat(), 12.0f, 1.0f);

    // 4. Draw Stem Label
    auto labelArea = pBounds.reduced(12);
    g.setColour(player->colour);
    g.setFont(juce::Font("JetBrains Mono", 10.0f, juce::Font::bold));
    g.drawText(player->name.toUpperCase(), labelArea.removeFromTop(16),
               juce::Justification::topLeft);

    // 5. Draw Simulated Waveform under the slider
    auto waveArea = labelArea.reduced(2);
    waveArea.removeFromLeft(70);  // skip buttons
    waveArea.removeFromRight(90); // skip download

    int barCount = 45;
    int barWidth = (waveArea.getWidth() - (barCount - 1) * 2) / barCount;
    if (barWidth < 1)
      barWidth = 1;
    int heights[] = {10, 18, 28, 14, 38, 22, 12, 30, 48, 16, 8,  26,
                     42, 20, 14, 32, 24, 12, 38, 44, 28, 10, 16, 22,
                     34, 12, 8,  20, 36, 14, 24, 40, 18, 10, 28, 44,
                     32, 12, 22, 18, 30, 14, 8,  16, 12};

    // Get active progress ratio to color the waveform active part differently!
    double currentPos = player->transportSource.getCurrentPosition();
    double totalLen = player->seekBar.getMaximum();
    double progressRatio = totalLen > 0.0 ? (currentPos / totalLen) : 0.0;
    int activeBars = (int)(barCount * progressRatio);

    for (int b = 0; b < barCount; ++b) {
      int h = heights[(b + i * 5) % 45] * waveArea.getHeight() / 50;
      if (b < activeBars)
        g.setColour(player->colour.withAlpha(0.6f)); // Active glowing progress
      else
        g.setColour(player->colour.withAlpha(0.12f)); // Background inactive

      g.fillRoundedRectangle((float)(waveArea.getX() + b * (barWidth + 2)),
                             (float)(waveArea.getCentreY() - h / 2 + 2),
                             (float)barWidth, (float)h, 1.0f);
    }

    yOffset += playerHeight + gap;
  }
}

void MainComponent::resized() {
  auto bounds = getLocalBounds().reduced(24);
  bounds.removeFromTop(60);

  if (currentState == AppState::ScreenSplash) {
    selectFileBtn.setVisible(false);
    progressBar.setVisible(false);
    progressLabel.setVisible(false);
    downloadAllBtn.setVisible(false);
    clearBtn.setVisible(false);
    cancelBtn.setVisible(false);
  } else if (currentState == AppState::ScreenUpload) {
    selectFileBtn.setVisible(true);
    progressBar.setVisible(false);
    progressLabel.setVisible(false);
    downloadAllBtn.setVisible(false);
    clearBtn.setVisible(false);
    cancelBtn.setVisible(false);

    // Bento grid sizes
    auto bentoArea = bounds.removeFromTop(bounds.getHeight() - 110);
    auto leftBentoWidth = (bentoArea.getWidth() * 7) / 10;
    auto leftCard = bentoArea.removeFromLeft(leftBentoWidth).reduced(8);

    // Position select file button perfectly inside upload card
    selectFileBtn.setBounds(leftCard.getCentreX() - 80,
                            leftCard.getCentreY() + 50, 160, 40);
    selectFileBtn.setColour(juce::TextButton::textColourOffId, colPrimary);
  } else if (currentState == AppState::ScreenProcessing) {
    selectFileBtn.setVisible(false);
    downloadAllBtn.setVisible(false);
    clearBtn.setVisible(false);

    auto area = getLocalBounds().reduced(24);
    area.removeFromTop(60);

    // Position progressLabel at the title area
    auto titleArea = area.removeFromTop(40);
    progressLabel.setBounds(titleArea);
    progressLabel.setVisible(true);

    // Position native progressBar perfectly inside bento progress card
    auto card = area.withSizeKeepingCentre(480, 260);
    auto cardContent = card.reduced(24);
    cardContent.removeFromTop(50); // top row
    cardContent.removeFromTop(10); // gap
    auto barArea = cardContent.removeFromTop(16);

    progressBar.setBounds(barArea);
    progressBar.setVisible(true);

    // Center-align the CANCEL OPERATION button just below the 480x260 bento
    // card
    cancelBtn.setBounds(card.getCentreX() - 100, card.getBottom() + 24, 200,
                        40);
    cancelBtn.setVisible(true);
  } else if (currentState == AppState::ScreenResults) {
    selectFileBtn.setVisible(false);
    progressBar.setVisible(false);
    progressLabel.setVisible(false);
    downloadAllBtn.setVisible(true);
    clearBtn.setVisible(true);
    cancelBtn.setVisible(false);

    bounds.removeFromTop(90); // skip headers

    int yOffset = bounds.getY() + 10;
    int playerHeight = 80;
    int gap = 16;

    for (auto &player : stemPlayers) {
      auto pBounds = juce::Rectangle<int>(bounds.getX(), yOffset,
                                          bounds.getWidth(), playerHeight);

      // Inner content margin
      auto inner = pBounds.reduced(12);
      inner.removeFromTop(16); // skip top label area

      // Play & Stop circular buttons on left (width == height == 40)
      auto btnArea = inner.removeFromLeft(90);
      player->playBtn.setBounds(
          btnArea.removeFromLeft(40).withSizeKeepingCentre(40, 40));
      player->playBtn.setColour(juce::TextButton::textColourOffId,
                                player->colour);

      btnArea.removeFromLeft(10); // gap

      player->stopBtn.setBounds(
          btnArea.removeFromLeft(40).withSizeKeepingCentre(40, 40));
      player->stopBtn.setColour(juce::TextButton::textColourOffId,
                                juce::Colour::fromString("#FF8B90A0"));

      // Download circular button on right
      auto dlArea = inner.removeFromRight(50);
      player->downloadBtn.setBounds(dlArea.withSizeKeepingCentre(40, 40));
      player->downloadBtn.setColour(juce::TextButton::textColourOffId,
                                    juce::Colours::white);

      // Seekbar in the middle
      player->seekBar.setBounds(inner.reduced(4, 0));

      yOffset += playerHeight + gap;
    }

    // Save All Zip button and Clear button at the very bottom
    downloadAllBtn.setBounds(bounds.getX(), yOffset + 10, bounds.getWidth(),
                             45);
    downloadAllBtn.setColour(juce::TextButton::textColourOffId, colPrimary);

    clearBtn.setBounds(bounds.getX(), yOffset + 60, bounds.getWidth(), 35);
  }
}

//==============================================================================
bool MainComponent::isInterestedInFileDrag(const juce::StringArray &files) {
  return currentState == AppState::ScreenUpload && files.size() == 1;
}

void MainComponent::filesDropped(const juce::StringArray &files, int /*x*/,
                                 int /*y*/) {
  if (currentState == AppState::ScreenUpload && files.size() == 1) {
    startProcessing(juce::File(files[0]));
  }
}

void MainComponent::openFileChooser() {
  static juce::FileChooser fc(
      "Select an audio file...",
      juce::File::getSpecialLocation(juce::File::userMusicDirectory),
      "*.wav;*.mp3;*.flac");

  fc.launchAsync(juce::FileBrowserComponent::openMode |
                     juce::FileBrowserComponent::canSelectFiles,
                 [this](const juce::FileChooser &chooser) {
                   if (chooser.getResults().size() > 0)
                     startProcessing(chooser.getResult());
                 });
}

//==============================================================================
void MainComponent::startProcessing(const juce::File &fileToProcess) {
  inputFile = fileToProcess;
  currentState = AppState::ScreenProcessing;
  processStatusText = "Initializing...";
  
  progressValue = 0.0;
  targetProgressValue = 0.0;
  resized();
  repaint();

  // Parar qualquer thread em background ativa antes de iniciar
  if (engineWorker != nullptr) {
    engineWorker->signalThreadShouldExit();
    engineWorker->waitForThreadToExit(2000);
    engineWorker.reset();
  }

  // Cria e inicia o worker para processar o áudio nativamente fora da Message Thread
  engineWorker = std::make_unique<StemEngineWorker>(
      StemEngineWorker::Job::ProcessTrack, [this]() {
        if (engineWorker != nullptr && engineWorker->wasSuccessful()) {
          finalizeProcessing();
        } else {
          juce::AlertWindow::showMessageBoxAsync(
              juce::AlertWindow::WarningIcon, "Erro de Processamento",
              "A extração de stems falhou ou foi interrompida.");
          currentState = AppState::ScreenUpload;
          resized();
          repaint();
        }
      });

  auto inputPath = inputFile.getFullPathName().toStdString();
  auto outputDir = tempDir.getFullPathName().toStdString();

  engineWorker->setProcessingParams(inputPath, outputDir, 
      [this](float progress) {
        juce::MessageManager::callAsync([this, progress]() {
          targetProgressValue = progress;
        });
      });

  engineWorker->startThread();
}

void MainComponent::timerCallback() {
  if (currentState == AppState::ScreenProcessing) {
    // Smoothly interpolate progressValue towards targetProgressValue
    if (progressValue < targetProgressValue) {
      double diff = targetProgressValue - progressValue;
      double step = diff * 0.08; // 8% interpolation factor for extra smoothness
      if (step < 0.001) {
        step = 0.001; // minimum step to guarantee reaching target
      }
      progressValue = std::min(targetProgressValue, progressValue + step);
    }

    // Construct progress status text based on the smoothly interpolated progressValue
    juce::String statusText = "Processando...";
    int percent = (int)std::round(progressValue * 100.0);
    if (percent < 0)
      percent = 0;
    if (percent > 100)
      percent = 100;

    if (percent > 0 && percent < 100) {
      statusText = "Separando Stems... " + juce::String(percent) + "%";
    } else if (percent == 100) {
      statusText = "Concluindo processamento...";
    } else {
      statusText = "Carregando áudio e inicializando engine de IA (Aguarde)...";
    }

    progressLabel.setText(statusText, juce::dontSendNotification);

    // Write output to debug log file
    juce::File logFile("C:\\StemsTriDJs\\process_log.txt");
    logFile.appendText(
        "Parsed progressValue: " + juce::String(progressValue * 100.0) +
        "% (" + statusText + ")\n");

    repaint();
  } else if (currentState == AppState::ScreenResults) {
    for (auto &player : stemPlayers) {
      if (player->transportSource.isPlaying() &&
          !player->seekBar.isMouseButtonDown()) {
        player->seekBar.setValue(player->transportSource.getCurrentPosition(),
                                 juce::dontSendNotification);
      }
    }
  }
}

void MainComponent::finalizeProcessing() {
  loadAudioFiles();
  currentState = AppState::ScreenResults;
  resized();
  repaint();
}

//==============================================================================
void MainComponent::loadAudioFiles() {
  sourcePlayer.setSource(nullptr);
  mixerSource.removeAllInputs();
  stemPlayers.clear();

  juce::String baseName = inputFile.getFileNameWithoutExtension();

  juce::File vocalsFile = tempDir.getChildFile(baseName + "_vocals.wav");
  juce::File drumsFile = tempDir.getChildFile(baseName + "_drums.wav");
  juce::File bassFile = tempDir.getChildFile(baseName + "_bass.wav");
  juce::File otherFile = tempDir.getChildFile(baseName + "_other.wav");

  auto setupPlayer = [this](const juce::String &name, const juce::File &file,
                            juce::Colour col) {
    auto player = std::make_unique<StemPlayer>();
    setupStemPlayer(*player, name, col, file);
    stemPlayers.push_back(std::move(player));
  };

  setupPlayer("Vocals", vocalsFile, colVocals);
  setupPlayer("Drums", drumsFile, colDrums);
  setupPlayer("Bass", bassFile, colBass);
  setupPlayer("Other", otherFile, colOther);

  sourcePlayer.setSource(&mixerSource);

  // Start updating the seek bars
  startTimer(40);
}

void MainComponent::setupStemPlayer(StemPlayer &player,
                                    const juce::String &name,
                                    juce::Colour colour,
                                    const juce::File &file) {
  player.name = name;
  player.audioFile = file;
  player.colour = colour;

  if (file.existsAsFile()) {
    auto *reader = formatManager.createReaderFor(file);
    if (reader != nullptr) {
      player.readerSource =
          std::make_unique<juce::AudioFormatReaderSource>(reader, true);
      player.transportSource.setSource(player.readerSource.get(), 0, nullptr,
                                       reader->sampleRate);
      mixerSource.addInputSource(&player.transportSource, false);

      // Set slider range in seconds
      player.seekBar.setRange(0.0,
                              reader->lengthInSamples / reader->sampleRate);
    }
  }

  addChildComponent(player.playBtn);
  player.playBtn.setButtonText(
      juce::CharPointer_UTF8("\xe2\x96\xb6")); // ▶ Play
  player.playBtn.setColour(juce::TextButton::buttonColourId, colSurface);
  player.playBtn.setColour(juce::TextButton::textColourOffId, colour);
  player.playBtn.onClick = [this, &player] { togglePlayPause(player); };

  addChildComponent(player.stopBtn);
  player.stopBtn.setButtonText(
      juce::CharPointer_UTF8("\xe2\x96\xa0")); // ■ Stop
  player.stopBtn.setColour(juce::TextButton::buttonColourId, colSurface);
  player.stopBtn.setColour(juce::TextButton::textColourOffId,
                           juce::Colour::fromString("#FF8B90A0"));
  player.stopBtn.onClick = [this, &player] {
    player.transportSource.stop();
    player.transportSource.setPosition(0.0);
    player.playBtn.setButtonText(juce::CharPointer_UTF8("\xe2\x96\xb6"));
    player.seekBar.setValue(0.0, juce::dontSendNotification);
  };

  addChildComponent(player.downloadBtn);
  player.downloadBtn.setButtonText(
      juce::CharPointer_UTF8("\xe2\x86\x93")); // ↓ Download
  player.downloadBtn.setColour(juce::TextButton::buttonColourId, colSurface);
  player.downloadBtn.setColour(juce::TextButton::textColourOffId,
                               juce::Colours::white);
  player.downloadBtn.onClick = [this, &player] {
    exportSingleStem(player.audioFile, player.name + ".wav");
  };

  addChildComponent(player.seekBar);
  player.seekBar.setSliderStyle(juce::Slider::LinearHorizontal);
  player.seekBar.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  player.seekBar.setColour(juce::Slider::trackColourId, colour.withAlpha(0.5f));
  player.seekBar.setColour(juce::Slider::backgroundColourId, colSurface);
  player.seekBar.setColour(juce::Slider::thumbColourId, colour);

  // Sync seeking across all tracks
  player.seekBar.onValueChange = [this, &player] {
    if (player.seekBar.isMouseButtonDown()) {
      double newPosition = player.seekBar.getValue();
      for (auto &otherPlayer : stemPlayers) {
        otherPlayer->transportSource.setPosition(newPosition);
      }
    }
  };

  player.playBtn.setVisible(true);
  player.stopBtn.setVisible(true);
  player.downloadBtn.setVisible(true);
  player.seekBar.setVisible(true);
}

void MainComponent::togglePlayPause(StemPlayer &player) {
  if (player.transportSource.isPlaying()) {
    player.transportSource.stop();
    player.playBtn.setButtonText(
        juce::CharPointer_UTF8("\xe2\x96\xb6")); // ▶ Play
  } else {
    // Perfect sync: start at the exact current position of any other active
    // player
    double syncPos = 0.0;
    for (auto &otherPlayer : stemPlayers) {
      if (otherPlayer->transportSource.isPlaying()) {
        syncPos = otherPlayer->transportSource.getCurrentPosition();
        break;
      }
    }
    player.transportSource.setPosition(syncPos);
    player.transportSource.start();
    player.playBtn.setButtonText(
        juce::CharPointer_UTF8("\xe2\x8f\xb8")); // ⏸ Pause
  }
}

void MainComponent::toggleAllPlayPause() {
  bool anyPlaying = false;
  for (auto &player : stemPlayers) {
    if (player->transportSource.isPlaying()) {
      anyPlaying = true;
      break;
    }
  }

  for (auto &player : stemPlayers) {
    if (anyPlaying) {
      player->transportSource.stop();
      player->playBtn.setButtonText(
          juce::CharPointer_UTF8("\xe2\x96\xb6")); // ▶ Play
    } else {
      // Sync all playing states
      player->transportSource.setPosition(0.0);
      player->transportSource.start();
      player->playBtn.setButtonText(
          juce::CharPointer_UTF8("\xe2\x8f\xb8")); // ⏸ Pause
    }
  }
}

void MainComponent::exportSingleStem(const juce::File &fileToExport,
                                     const juce::String &defaultName) {
  if (!fileToExport.existsAsFile())
    return;

  static juce::FileChooser fc(
      "Save Stem",
      juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
          .getChildFile(defaultName),
      "*.wav");

  fc.launchAsync(juce::FileBrowserComponent::saveMode |
                     juce::FileBrowserComponent::canSelectFiles,
                 [fileToExport](const juce::FileChooser &chooser) {
                   auto targetFile = chooser.getResult();
                   if (targetFile != juce::File()) {
                     targetFile.deleteFile();
                     fileToExport.copyFileTo(targetFile);
                   }
                 });
}

void MainComponent::exportAllAsZip() {
  juce::String zipName = inputFile.getFileNameWithoutExtension() + "_Stems.zip";
  static juce::FileChooser fc(
      "Save All Stems",
      juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
          .getChildFile(zipName),
      "*.zip");

  fc.launchAsync(juce::FileBrowserComponent::saveMode |
                     juce::FileBrowserComponent::canSelectFiles,
                 [this](const juce::FileChooser &chooser) {
                   auto targetZip = chooser.getResult();
                   if (targetZip != juce::File()) {
                     targetZip.deleteFile();
                     juce::ZipFile::Builder builder;

                     for (auto &player : stemPlayers) {
                       if (player->audioFile.existsAsFile()) {
                         builder.addFile(player->audioFile, 9,
                                         player->audioFile.getFileName());
                       }
                     }

                     std::unique_ptr<juce::FileOutputStream> outStream(
                         targetZip.createOutputStream());
                     if (outStream != nullptr) {
                       builder.writeToStream(*outStream, nullptr);
                       juce::AlertWindow::showMessageBoxAsync(
                           juce::AlertWindow::InfoIcon, "Success",
                           "All stems compressed successfully.");
                     }
                    }
                  });
}
