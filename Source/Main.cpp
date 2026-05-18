#include <JuceHeader.h>
#include "MainComponent.h"
#include "StemEngine.h"
#include <algorithm>



class TriDJsStemsApplication  : public juce::JUCEApplication
{
public:
    TriDJsStemsApplication() {}

    const juce::String getApplicationName() override       { return "TriDJs_Separador_Stems"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    class SplashWindow : public juce::DocumentWindow,
                         public juce::Timer
    {
    public:
        SplashWindow (juce::Image image)
            : DocumentWindow ("TriDJs Stems", juce::Colours::transparentBlack, 0),
              splashImage (image)
        {
            setUsingNativeTitleBar (false);
            setSize (500, 500);
            centreWithSize (500, 500);
            
            // Inicia totalmente transparente para fazer o fade-in cinematográfico
            setAlpha (0.0f);
            setVisible (true);
            setAlwaysOnTop (true);

            // Inicia o timer de animação a 60 FPS para máxima fluidez nas animações
            startTimerHz (60);
        }

        ~SplashWindow() override
        {
            stopTimer();
        }

        void paint (juce::Graphics& g) override
        {
            // 1. Fundo preto absoluto cinematográfico
            g.fillAll (juce::Colour::fromString("#FF050505"));

            // 2. Desenha o logotipo centralizado na metade superior
            if (splashImage.isValid())
            {
                g.drawImageWithin (splashImage, 0, 15, 500, 270, 
                                   juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
            }

            // 3. Glow neon verde suave de 2px de espessura nas bordas da janela borderless
            g.setColour (juce::Colour::fromString("#FF01F5A0").withAlpha(0.18f));
            g.drawRect (getLocalBounds().toFloat(), 2.0f);

            // 4. Desenha o Terminal Virtual no canto inferior esquerdo (Fidelidade total ao print)
            int textX = 45;
            int textYStart = 295;
            int lineHeight = 20;

            auto status = StemEngine::getInstance().getStatus();

            if (!isReady)
            {
                // ESTADO DE CARREGAMENTO REAL DA ENGINE + TIMING ANIMAÇÃO
                int numLinesToShow = 1;
                if (status == StemEngine::Status::Booting) numLinesToShow = 1;
                else if (status == StemEngine::Status::CudaLoading) numLinesToShow = 2;
                else if (status == StemEngine::Status::ModelLoading) numLinesToShow = 3;
                else if (status == StemEngine::Status::WarmingUp) numLinesToShow = 5;
                else if (status == StemEngine::Status::Failed) numLinesToShow = 5;

                // Também avança numLinesToShow com os ticks para manter uma animação mínima charmosa no início
                int tickLines = 1;
                if (loaderTicks >= 160)      tickLines = 5;
                else if (loaderTicks >= 120) tickLines = 4;
                else if (loaderTicks >= 80)  tickLines = 3;
                else if (loaderTicks >= 40)  tickLines = 2;

                numLinesToShow = std::max(numLinesToShow, tickLines);

                juce::String prefixes[] = { "[...]", "[...]", "[...]", "[...]", "[...]" };
                juce::String messages[] = {
                    "Initializing Neural Engine...",
                    "Loading CUDA Runtime...",
                    "Loading AI Model (168MB)...",
                    "Allocating VRAM...",
                    "Warming Up Inference Core..."
                };

                for (int i = 0; i < numLinesToShow; ++i)
                {
                    int currentY = textYStart + (i * lineHeight);

                    // Desenha o prefixo roxo/azul futurista "[...]"
                    g.setColour (juce::Colour::fromString("#FFBD80FF"));
                    g.setFont (juce::Font ("Consolas", 12.0f, juce::Font::bold));
                    g.drawText (prefixes[i], textX, currentY, 50, lineHeight, juce::Justification::centredLeft);

                    // Desenha a mensagem de log
                    g.setColour (juce::Colour::fromString("#FFCCCCCC"));
                    g.setFont (juce::Font ("Consolas", 12.0f, juce::Font::plain));
                    g.drawText (messages[i], textX + 50, currentY, 360, lineHeight, juce::Justification::centredLeft);
                }

                if (status == StemEngine::Status::Failed)
                {
                    g.setColour (juce::Colour::fromString("#FFFF4D4D"));
                    g.setFont (juce::Font ("Consolas", 11.0f, juce::Font::bold));
                    g.drawText ("Initialization Failed!", 0, 445, 500, 20, juce::Justification::centred);
                }
                else
                {
                    // Subtítulo minimalista no rodapé
                    g.setColour (juce::Colour::fromString("#FF404040"));
                    g.setFont (juce::Font ("Consolas", 9.5f, juce::Font::plain));
                    g.drawText ("AI STEM SEPARATION ENGINE", 0, 465, 500, 20, juce::Justification::centred);
                }
            }
            else
            {
                // ESTADO PRONTO (READY) - Linhas com "[OK]" verde neon brilhante
                juce::String prefixes[] = { "[OK]", "[OK]", "[OK]", "[OK]" };
                juce::String messages[] = {
                    "CUDA Runtime Loaded",
                    "AI Model Cached",
                    "VRAM Allocated",
                    "Stem Engine Ready"
                };

                for (int i = 0; i < 4; ++i)
                {
                    int currentY = textYStart + (i * lineHeight);

                    // Desenha o prefixo "[OK]" em verde neon brilhante
                    g.setColour (juce::Colour::fromString("#FF01F5A0"));
                    g.setFont (juce::Font ("Consolas", 12.0f, juce::Font::bold));
                    g.drawText (prefixes[i], textX, currentY, 50, lineHeight, juce::Justification::centredLeft);

                    // Desenha a mensagem de sucesso
                    g.setColour (juce::Colour::fromString("#FFEAEAEA"));
                    g.setFont (juce::Font ("Consolas", 12.0f, juce::Font::plain));
                    g.drawText (messages[i], textX + 50, currentY, 360, lineHeight, juce::Justification::centredLeft);
                }

                // Texto em destaque brilhante "READY" no centro do rodapé
                g.setColour (juce::Colour::fromString("#FF01F5A0"));
                g.setFont (juce::Font ("Consolas", 18.0f, juce::Font::bold));
                g.drawText ("READY", 0, 445, 500, 30, juce::Justification::centred);
            }
        }

        void setReady (bool success)
        {
            if (success)
            {
                isReady = true;
                readyHoldTicks = 36; // Mantém a tela por ~600ms no estado READY para impacto de carregamento
                repaint();
            }
            else
            {
                // Se falhar ou der timeout, inicia fade-out imediatamente para exibir o erro do app principal
                isFadingOut = true;
            }
        }

        void startFadeOut (std::function<void()> onCompleteCallback)
        {
            onFadeOutComplete = onCompleteCallback;
            // Se não formos bem-sucedidos (crashed/timeout), força o início do fade-out
            if (!isReady)
                isFadingOut = true;
        }

        void timerCallback() override
        {
            float currentAlpha = getAlpha();
            
            if (isFadingOut)
            {
                if (currentAlpha > 0.0f)
                {
                    // Fade-out suave de 0.6s (decresce 0.035 por frame a 60 FPS)
                    setAlpha (std::max (0.0f, currentAlpha - 0.035f));
                }
                else
                {
                    stopTimer();
                    if (onFadeOutComplete != nullptr)
                        onFadeOutComplete();
                }
            }
            else if (isReady)
            {
                if (readyHoldTicks > 0)
                {
                    readyHoldTicks--;
                }
                else
                {
                    isFadingOut = true; // Inicia o fade-out após segurar a tela READY
                }
            }
            else
            {
                // Fade-in suave inicial de 0.6s (acresce 0.035 por frame a 60 FPS)
                if (currentAlpha < 1.0f)
                {
                    setAlpha (std::min (1.0f, currentAlpha + 0.035f));
                }

                // Incrementa ticks do carregador para progressão de linhas
                loaderTicks++;
                if (loaderTicks % 2 == 0) // Repinta a tela com maior frequência de atualização de status
                    repaint();

                // Checa o status da engine em background em tempo real
                auto status = StemEngine::getInstance().getStatus();
                if (status == StemEngine::Status::Ready)
                {
                    setReady (true);
                }
                else if (status == StemEngine::Status::Failed)
                {
                    setReady (false);
                }
            }
        }

    private:
        juce::Image splashImage;
        int loaderTicks = 0;
        int readyHoldTicks = 0;
        bool isReady = false;
        bool isFadingOut = false;
        std::function<void()> onFadeOutComplete;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SplashWindow)
    };

    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name)
            : DocumentWindow (name,
                              juce::Colours::darkgrey,
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            
            // Define o ícone da janela usando logo.png
            juce::File iconFile("C:\\StemsTriDJs\\logo.png");
            if (!iconFile.existsAsFile())
                iconFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("resources").getChildFile("logo.png");
            if (!iconFile.existsAsFile())
                iconFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getChildFile("logo.png");
            if (!iconFile.existsAsFile())
                iconFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("logo.png");
            
            if (iconFile.existsAsFile())
            {
                juce::Image iconImg = juce::ImageFileFormat::loadFrom(iconFile);
                if (iconImg.isValid())
                    setIcon (iconImg);
            }
            
            auto* mainComp = new MainComponent();
            
            // Vincula o callback de inicialização
            mainComp->onInitializationComplete = [this] (bool success) {
                auto* app = dynamic_cast<TriDJsStemsApplication*> (juce::JUCEApplication::getInstance());
                if (app != nullptr)
                    app->dismissSplashAndShowMainWindow (success);
            };
            
            mainComp->startWarmup();
            
            setContentOwned (mainComp, true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (false); // Oculto no boot, visível somente após o fade-out
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    void initialise (const juce::String& commandLine) override
    {
        // 1. Carrega o arquivo principal logo.png do TriDJs
        juce::File splashFile("C:\\StemsTriDJs\\logo.png");
        if (!splashFile.existsAsFile())
            splashFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("resources").getChildFile("logo.png");
        if (!splashFile.existsAsFile())
            splashFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getChildFile("logo.png");
        if (!splashFile.existsAsFile())
            splashFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("logo.png");
        
        juce::Image splashImg;
        if (splashFile.existsAsFile())
        {
            splashImg = juce::ImageFileFormat::loadFrom(splashFile);
        }
        else
        {
            // Fallback amigável para splash.png caso logo.png não esteja na pasta
            juce::File fallbackFile("C:\\StemsTriDJs\\splash.png");
            if (fallbackFile.existsAsFile())
                splashImg = juce::ImageFileFormat::loadFrom(fallbackFile);
        }

        // 2. Exibe o Splash cinematográfico 500x500 borderless
        splashWindow.reset (new SplashWindow (splashImg));

        // 3. Instancia a janela principal oculta nos bastidores
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        splashWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
    }

    void dismissSplashAndShowMainWindow (bool success)
    {
        if (splashWindow != nullptr)
        {
            // 1. Aciona o estado de READY com animação e hold de 600ms
            splashWindow->setReady (success);

            // 2. Registra o fade-out e exibe a janela principal ao final do fade-out
            splashWindow->startFadeOut ([this] {
                juce::MessageManager::callAsync ([this] {
                    splashWindow = nullptr; // Libera o splash da memória com segurança no próximo loop do MessageManager
                    if (mainWindow != nullptr)
                    {
                        mainWindow->setVisible (true);
                        mainWindow->toFront (true);
                    }
                });
            });
        }
    }

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<SplashWindow> splashWindow;
};

START_JUCE_APPLICATION (TriDJsStemsApplication)
