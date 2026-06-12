#pragma once

#include <atomic>
#include <functional>
#include <optional>

#include <juce_data_structures/juce_data_structures.h>

namespace conduit
{

//==============================================================================
/**
    Kontrakt der Resize-Policy gegenüber dem CaptureService.

    Als Interface herausgelöst, damit die Zustandslogik der Policy ohne
    echten Service (und ohne echte Puffer-Allokation) testbar bleibt.
    Alle Methoden laufen auf dem Message Thread.
*/
class ICaptureBufferHost
{
public:
    virtual ~ICaptureBufferHost() = default;

    /** true, wenn irgendein Capture-Kanal aktiv ist (Gate offen oder "held"). */
    [[nodiscard]] virtual bool isAnyChannelActive() const = 0;

    /** Gates schließen, Puffer verwerfen — bewusst KEIN Auto-Export. */
    virtual void invalidateAllBuffers() = 0;

    /** Puffer anhand der aktuellen Settings neu allokieren. */
    virtual void reallocateBuffers() = 0;
};

//==============================================================================
/**
    Capture-Settings — App-Zustand, KEIN Patch-Zustand.

    Persistenz via juce::ApplicationProperties (App-Name "Conduit"), bewusst
    NICHT im Root-ValueTree: loadPreset() ersetzt den Root-Tree, die
    Capture-Konfiguration bleibt davon unberührt — dieselbe Trennung wie
    beim Link-Tempo (Session-Zustand, siehe EngineEditor-Doku).

    Threading:
    - Setter und alles mit File-Zugriff: Message Thread
    - RT-relevante Felder (thresholdDb, holdMinutes, preRollSeconds,
      autoCalibrate, bufferMinutes) sind als std::atomic publiziert
      [Message → Audio]; die Getter sind von jedem Thread lesbar
    - UI-Benachrichtigung über juce::ChangeBroadcaster (async)

    Resize-Policy (bufferMinutes / preRollSeconds):
    Beide Werte bestimmen die Ring-Dimensionierung — eine Übernahme bei
    laufender Aufzeichnung würde gepuffertes Material verwerfen. Deshalb:

    1. Kein Kanal aktiv (laut ICaptureBufferHost) → stille Übernahme,
       danach reallocateBuffers(). Ohne registrierten Host gilt "inaktiv".
    2. Kanal aktiv (Gate offen oder "held") → der Wert wird NICHT
       übernommen; stattdessen feuert onPendingResize mit einem
       PendingResizeRequest. Die UI bestätigt ASYNC — wegen
       JUCE_MODAL_LOOPS_PERMITTED=0 als AlertWindow mit Callback,
       Muster wie showMessageBoxAsync in EngineEditor.cpp — und ruft
       dann confirmPendingResize() bzw. cancelPendingResize().
    3. confirmPendingResize() → invalidateAllBuffers() (Gates schließen,
       Puffer verwerfen, kein Auto-Export), Wert übernehmen, dann
       reallocateBuffers(). Alles auf dem Message Thread.

    Ein neuer Resize-Versuch während einer offenen Anfrage ersetzt diese
    (latest wins).
*/
class CaptureSettings : public juce::ChangeBroadcaster
{
public:
    //==========================================================================
    // Ranges & Defaults (public für UI-Slider und Tests)
    static constexpr int   minBufferMinutes      = 5;
    static constexpr int   maxBufferMinutes      = 30;
    static constexpr int   defaultBufferMinutes  = 15;

    static constexpr int   minPreRollSeconds     = 10;
    static constexpr int   maxPreRollSeconds     = 120;
    static constexpr int   defaultPreRollSeconds = 60;

    static constexpr float minThresholdDb        = -80.0f;
    static constexpr float maxThresholdDb        = -20.0f;
    static constexpr float defaultThresholdDb    = -40.0f;

    static constexpr int   minHoldMinutes        = 1;
    static constexpr int   maxHoldMinutes        = 30;
    static constexpr int   defaultHoldMinutes    = 10;

    static constexpr bool  defaultAutoCalibrate  = true;

    static constexpr int   minRamLimitGb         = 1;
    static constexpr int   maxRamLimitGb         = 64;
    static constexpr int   defaultRamLimitGb     = 3;

    static constexpr int   defaultExportBitDepth = 24;  // erlaubt: 16 / 24 / 32

    //==========================================================================
    /** Persistenz-Ziel der App: ApplicationProperties mit App-Name "Conduit". */
    [[nodiscard]] static juce::PropertiesFile::Options defaultOptions();

    /** Benutzer-Musik/Conduit Captures. */
    [[nodiscard]] static juce::File defaultExportDirectory();

    /** Tests injizieren eigene Options (Temp-Verzeichnis). */
    explicit CaptureSettings (const juce::PropertiesFile::Options& options = defaultOptions());
    ~CaptureSettings() override;

    //==========================================================================
    // Getter [beliebiger Thread, lock-free]
    [[nodiscard]] int   getBufferMinutes() const noexcept;
    [[nodiscard]] int   getPreRollSeconds() const noexcept;
    [[nodiscard]] float getThresholdDb() const noexcept;
    [[nodiscard]] int   getHoldMinutes() const noexcept;
    [[nodiscard]] bool  getAutoCalibrate() const noexcept;
    [[nodiscard]] int   getRamLimitGb() const noexcept;
    [[nodiscard]] int   getExportBitDepth() const noexcept;

    /** [Message Thread] */
    [[nodiscard]] juce::File getExportDirectory() const;

    //==========================================================================
    // Setter [Message Thread] — clampen, persistieren, ChangeBroadcast
    void setThresholdDb (float db);
    void setHoldMinutes (int minutes);
    void setAutoCalibrate (bool enabled);
    void setRamLimitGb (int gigabytes);
    void setExportDirectory (const juce::File& directory);  // ungültig/leer → Default
    void setExportBitDepth (int bits);                      // nicht 16/24/32 → Default

    //==========================================================================
    // Resize-Policy (siehe Klassendoku)
    enum class ResizeOutcome { applied, pendingConfirmation };

    struct PendingResizeRequest
    {
        enum class Field { bufferMinutes, preRollSeconds };

        Field field;
        int currentValue;
        int requestedValue;
    };

    /** [Message Thread] applied = übernommen (still), pendingConfirmation =
        Kanal aktiv, onPendingResize wurde gefeuert, Wert unverändert. */
    ResizeOutcome setBufferMinutes (int minutes);
    ResizeOutcome setPreRollSeconds (int seconds);

    /** [Message Thread] UI hat bestätigt: invalidateAllBuffers() → Wert
        übernehmen → reallocateBuffers(). No-op ohne offene Anfrage. */
    void confirmPendingResize();

    /** [Message Thread] UI hat abgelehnt: Anfrage verwerfen, Wert bleibt. */
    void cancelPendingResize();

    [[nodiscard]] bool hasPendingResize() const noexcept;
    [[nodiscard]] std::optional<PendingResizeRequest> getPendingResize() const;

    /** Wird bei aktivem Kanal statt der Übernahme aufgerufen (Message Thread). */
    std::function<void (const PendingResizeRequest&)> onPendingResize;

    /** EngineProcessor verdrahtet den CaptureService; nullptr = "inaktiv". */
    void setBufferHost (ICaptureBufferHost* host) noexcept;

    //==========================================================================
    /** [Message Thread] Ausstehende Änderungen sofort auf Platte schreiben. */
    void flush();

private:
    void loadFromFile();
    void writeValue (const char* key, const juce::var& value);
    ResizeOutcome requestRingResize (PendingResizeRequest::Field field, int requestedValue);
    void applyRingValue (PendingResizeRequest::Field field, int value);

    juce::ApplicationProperties applicationProperties;
    ICaptureBufferHost* bufferHost = nullptr;
    std::optional<PendingResizeRequest> pendingResize;

    // RT-relevante Felder [Message schreibt, Audio liest]
    std::atomic<int>   bufferMinutes  { defaultBufferMinutes };
    std::atomic<int>   preRollSeconds { defaultPreRollSeconds };
    std::atomic<float> thresholdDb    { defaultThresholdDb };
    std::atomic<int>   holdMinutes    { defaultHoldMinutes };
    std::atomic<bool>  autoCalibrate  { defaultAutoCalibrate };

    // Nur-MT-Felder (atomic für konsistente Getter-Signaturen)
    std::atomic<int>   ramLimitGb     { defaultRamLimitGb };
    std::atomic<int>   exportBitDepth { defaultExportBitDepth };
    juce::String exportDirectoryPath;  // nur Message Thread

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaptureSettings)
};

} // namespace conduit
