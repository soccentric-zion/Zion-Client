#include "app.h"

#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

#include "client_service.pb.h"
#include "registry/AdapterRegistry.h"
#include "hal_service/HalServiceImpl.h"

#include <grpcpp/grpcpp.h>

// ─── Real adapters (35 modules with HAL source) ───────────────────────────
#include "registry/adapters/audio_interfaces_adapter.h"
#include "registry/adapters/camera_interfaces_adapter.h"
#include "registry/adapters/camera_module3_adapter.h"
#include "registry/adapters/clock_and_timing_adapter.h"
#include "registry/adapters/communication_interface_adapter.h"
#include "registry/adapters/cpu_adapter.h"
#include "registry/adapters/debug_and_development_interfaces_adapter.h"
#include "registry/adapters/deep_learning_accelerator_adapter.h"
#include "registry/adapters/display_interfaces_adapter.h"
#include "registry/adapters/environmental_adapter.h"
#include "registry/adapters/ethernet_adapter.h"
#include "registry/adapters/ethernet_switch_adapter.h"
#include "registry/adapters/external_connector_adapter.h"
#include "registry/adapters/functional_safety_adapter.h"
#include "registry/adapters/gnss_adapter.h"
#include "registry/adapters/gpio_adapter.h"
#include "registry/adapters/gpu_adapter.h"
#include "registry/adapters/imu_adapter.h"
#include "registry/adapters/mcu_interface_adapter.h"
#include "registry/adapters/pcie_adapter.h"
#include "registry/adapters/poe_hat_adapter.h"
#include "registry/adapters/power_management_adapter.h"
#include "registry/adapters/reliability_adapter.h"
#include "registry/adapters/security_features_adapter.h"
#include "registry/adapters/sense_hat_adapter.h"
#include "registry/adapters/serializer_deserializer_adapter.h"
#include "registry/adapters/software_interface_adapter.h"
#include "registry/adapters/storage_adapter.h"
#include "registry/adapters/system_integration_adapter.h"
#include "registry/adapters/system_memory_adapter.h"
#include "registry/adapters/thermal_adapter.h"
#include "registry/adapters/usb_adapter.h"
#include "registry/adapters/video_encode_decode_adapter.h"
#include "registry/adapters/vision_accelerator_adapter.h"
#include "registry/adapters/watchdog_and_reset_adapter.h"

// ─── Phase 3 — 16 new real adapters (DISABLED: HAL libs not yet implemented) ──
// #include "registry/adapters/accelerometer_adapter.h"
// #include "registry/adapters/analog_input_adapter.h"
// #include "registry/adapters/battery_adapter.h"
// #include "registry/adapters/bluetooth_adapter.h"
// #include "registry/adapters/can_bus_adapter.h"
// #include "registry/adapters/emmc_adapter.h"
// #include "registry/adapters/i2c_adapter.h"
// #include "registry/adapters/mipi_csi_adapter.h"
// #include "registry/adapters/mipi_dsi_adapter.h"
// #include "registry/adapters/nvme_adapter.h"
// #include "registry/adapters/pwm_output_adapter.h"
// #include "registry/adapters/rs485_adapter.h"
// #include "registry/adapters/rtc_adapter.h"
// #include "registry/adapters/sd_card_adapter.h"
// #include "registry/adapters/spi_adapter.h"
// #include "registry/adapters/uart_adapter.h"

// ─── Honest-stub for any unrecognised modules ─────────────────────────────
#include "registry/adapters/stub_adapter.h"

using namespace acadia::stream;

namespace zion::client {

// ---------------------------------------------------------------------------
// build_adapter_registry — registers all known module adapters.
// Adapters self-register here; no switch(module_id) anywhere else.
// ---------------------------------------------------------------------------
static std::unique_ptr<AdapterRegistry> build_adapter_registry(
    const std::vector<ModuleEntry>& all_modules)
{
    auto reg = std::make_unique<AdapterRegistry>();

    // ── Real adapters — 35 modules with HAL C++ source ────────────────────
    reg->register_adapter(std::make_unique<AudioInterfacesAdapter>());
    reg->register_adapter(std::make_unique<CameraInterfacesAdapter>());
    reg->register_adapter(std::make_unique<CameraModule3Adapter>());
    reg->register_adapter(std::make_unique<ClockAndTimingAdapter>());
    reg->register_adapter(std::make_unique<CommunicationInterfaceAdapter>());
    reg->register_adapter(std::make_unique<CpuAdapter>());
    reg->register_adapter(std::make_unique<DebugAndDevelopmentInterfacesAdapter>());
    reg->register_adapter(std::make_unique<DeepLearningAcceleratorAdapter>());
    reg->register_adapter(std::make_unique<DisplayInterfacesAdapter>());
    reg->register_adapter(std::make_unique<EnvironmentalAdapter>());
    reg->register_adapter(std::make_unique<EthernetAdapter>());
    reg->register_adapter(std::make_unique<EthernetSwitchAdapter>());
    reg->register_adapter(std::make_unique<ExternalConnectorAdapter>());
    reg->register_adapter(std::make_unique<FunctionalSafetyAdapter>());
    reg->register_adapter(std::make_unique<GnssAdapter>());
    reg->register_adapter(std::make_unique<GpioAdapter>());
    reg->register_adapter(std::make_unique<GpuAdapter>());
    reg->register_adapter(std::make_unique<ImuAdapter>());
    reg->register_adapter(std::make_unique<McuInterfaceAdapter>());
    reg->register_adapter(std::make_unique<PcieAdapter>());
    reg->register_adapter(std::make_unique<PoeHatAdapter>());
    reg->register_adapter(std::make_unique<PowerManagementAdapter>());
    reg->register_adapter(std::make_unique<ReliabilityAdapter>());
    reg->register_adapter(std::make_unique<SecurityFeaturesAdapter>());
    reg->register_adapter(std::make_unique<SenseHatAdapter>());
    reg->register_adapter(std::make_unique<SerializerDeserializerAdapter>());
    reg->register_adapter(std::make_unique<SoftwareInterfaceAdapter>());
    reg->register_adapter(std::make_unique<StorageAdapter>());
    reg->register_adapter(std::make_unique<SystemIntegrationAdapter>());
    reg->register_adapter(std::make_unique<SystemMemoryAdapter>());
    reg->register_adapter(std::make_unique<ThermalAdapter>());
    reg->register_adapter(std::make_unique<UsbAdapter>());
    reg->register_adapter(std::make_unique<VideoEncodeDecodeAdapter>());
    reg->register_adapter(std::make_unique<VisionAcceleratorAdapter>());
    reg->register_adapter(std::make_unique<WatchdogAndResetAdapter>());

    // ── Phase 3 — 16 new real adapters (DISABLED: HAL libs not yet implemented) ──
    // reg->register_adapter(std::make_unique<AccelerometerAdapter>());
    // reg->register_adapter(std::make_unique<AnalogInputAdapter>());
    // reg->register_adapter(std::make_unique<BatteryAdapter>());
    // reg->register_adapter(std::make_unique<BluetoothAdapter>());
    // reg->register_adapter(std::make_unique<CanBusAdapter>());
    // reg->register_adapter(std::make_unique<EmmcAdapter>());
    // reg->register_adapter(std::make_unique<I2cAdapter>());
    // reg->register_adapter(std::make_unique<MipiCsiAdapter>());
    // reg->register_adapter(std::make_unique<MipiDsiAdapter>());
    // reg->register_adapter(std::make_unique<NvmeAdapter>());
    // reg->register_adapter(std::make_unique<PwmOutputAdapter>());
    // reg->register_adapter(std::make_unique<Rs485Adapter>());
    // reg->register_adapter(std::make_unique<RtcAdapter>());
    // reg->register_adapter(std::make_unique<SdCardAdapter>());
    // reg->register_adapter(std::make_unique<SpiAdapter>());
    // reg->register_adapter(std::make_unique<UartAdapter>());

    // ── Honest stubs — for any truly unrecognised module IDs ─────────────
    // Returns SKIPPED with an explanatory message, NOT a fake pass.
    for (const auto& m : all_modules) {
        if (!reg->has_adapter(m.id)) {
            reg->register_adapter(std::make_unique<StubAdapter>(m.id));
        }
    }
    return reg;
}

// ---------------------------------------------------------------------------
// App ctor / dtor
// ---------------------------------------------------------------------------

App::App(config::ClientConfig cfg) : cfg_(std::move(cfg)) {}
App::~App() = default;

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

bool App::init() {
    spdlog::info("app: init client_id={} server={}:{}",
        cfg_.client_id, cfg_.server_address, cfg_.server_port);

    // 1. Open spool
    try {
        spool_ = std::make_unique<Spool>(
            cfg_.spool_path,
            cfg_.result_spool_cap_bytes,
            cfg_.log_spool_cap_bytes);
    } catch (const std::exception& e) {
        spdlog::critical("app: spool init failed: {}", e.what());
        return false;
    }

    // 2. Module registry
    registry_ = std::make_unique<ModuleRegistry>();
    if (!cfg_.modules_yaml_path.empty()) {
        try {
            int n = registry_->load(cfg_.modules_yaml_path);
            spdlog::info("app: loaded {} modules", n);
        } catch (const std::exception& e) {
            spdlog::warn("app: module registry load failed: {} (continuing)", e.what());
        }
    }

    // 3. Stream client
    StreamClientConfig scfg;
    scfg.server_address = cfg_.server_address;
    scfg.server_port    = cfg_.server_port;
    scfg.tls_ca         = cfg_.tls_ca;
    scfg.tls_cert       = cfg_.tls_cert;
    scfg.tls_key        = cfg_.tls_key;

    stream_ = std::make_unique<StreamClient>(scfg,
        [this](const ServerMessage& msg){ on_server_message(msg); });

    // 4. Run orchestrator — wired to the adapter registry.
    auto adapter_registry = build_adapter_registry(registry_->all());
    // Capture by shared ownership so the lambda outlives this scope.
    auto shared_registry = std::shared_ptr<AdapterRegistry>(std::move(adapter_registry));
    HalInvokeFn hal_fn = [shared_registry](const std::string& module_id,
                                            int mode,
                                            const std::vector<uint8_t>& cfg)
                                            -> std::vector<hal::TestResult> {
        return shared_registry->invoke(module_id, mode, cfg);
    };
    orchestrator_ = std::make_unique<RunOrchestrator>(
        *spool_, *stream_, std::move(hal_fn));

    // 4b. HalService gRPC server — shares the same adapter registry (invariant 0.2).
    //     Runs on hal_service_port; no DB writes on this path (invariant 0.3).
    {
        const std::string listen_addr =
            cfg_.hal_service_bind + ":" + std::to_string(cfg_.hal_service_port);

        auto hal_svc_impl = std::make_unique<hal_service::HalServiceImpl>(
            shared_registry, *registry_);

        ::grpc::ServerBuilder builder;
        builder.AddListeningPort(listen_addr, ::grpc::InsecureServerCredentials());
        builder.RegisterService(hal_svc_impl.release());  // gRPC takes ownership
        hal_service_server_ = builder.BuildAndStart();
        if (!hal_service_server_) {
            spdlog::critical("app: failed to start HalService on {}", listen_addr);
            return false;
        }
        spdlog::info("app: HalService listening on {} (no DB writes)", listen_addr);
    }

    // 5. Log collector
    LogCollectorConfig lccfg;
    lccfg.extra_paths = cfg_.log_paths;
    log_collector_ = std::make_unique<LogCollector>(*spool_, *stream_, lccfg);

    // 6. Janitor
    JanitorConfig jcfg;
    jcfg.board_id = cfg_.client_id;
    janitor_ = std::make_unique<Janitor>(*spool_, *stream_, jcfg);

    // 7. Diagnostic socket (T045)
    diag_socket_ = std::make_unique<DiagSocket>(
        "/run/zion/client.sock",
        [this](const std::string& /*query*/) -> std::string {
            auto states = spool_->all_run_states();
            std::string r = R"({"connected":)";
            r += stream_->is_connected() ? "true" : "false";
            r += R"(,"spool_bytes":)" + std::to_string(spool_->result_bytes_used());
            r += R"(,"active_runs":)" + std::to_string(states.size());
            r += R"(,"client_version":")" ZION_CLIENT_VERSION "\"";
            r += "}";
            return r;
        });

    return true;
}

// ---------------------------------------------------------------------------
// run
// ---------------------------------------------------------------------------

void App::run(std::atomic<bool>& shutdown) {
    spdlog::info("app: starting subsystems");

    stream_->start();
    log_collector_->start();
    janitor_->start();
    try {
        diag_socket_->start();
    } catch (const std::exception& e) {
        spdlog::warn("app: diag socket failed: {} (continuing)", e.what());
    }

    // Wait for stream to connect, then send Hello
    for (int i = 0; i < 30 && !stream_->is_connected(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (stream_->is_connected()) {
        send_hello();
        send_unacked_replay();
    }

    spdlog::info("app: running — waiting for SIGTERM/SIGINT");
    while (!shutdown.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Re-send Hello + replay if we (re)connected
        static bool was_connected = false;
        bool is_now = stream_->is_connected();
        if (is_now && !was_connected) {
            send_hello();
            send_unacked_replay();
        }
        was_connected = is_now;
    }

    spdlog::info("app: shutdown — stopping subsystems");
    // Shut down HalService with a grace period before stopping everything else.
    if (hal_service_server_) {
        hal_service_server_->Shutdown(
            std::chrono::system_clock::now() + std::chrono::seconds(5));
        hal_service_server_.reset();
    }
    diag_socket_->stop();
    janitor_->stop();
    log_collector_->stop();
    orchestrator_->stop_all();
    stream_->stop();
    spdlog::info("app: stopped");
}

// ---------------------------------------------------------------------------
// on_server_message
// ---------------------------------------------------------------------------

void App::on_server_message(const ServerMessage& msg) {
    switch (msg.payload_case()) {
        case ServerMessage::kConfigureRun:
            orchestrator_->on_configure(msg.configure_run());
            break;
        case ServerMessage::kStartRun:
            orchestrator_->on_start(msg.start_run().run_id());
            log_collector_->set_run_id(msg.start_run().run_id());
            break;
        case ServerMessage::kPauseRun:
            orchestrator_->on_pause(msg.pause_run().run_id());
            break;
        case ServerMessage::kResumeRun:
            orchestrator_->on_resume(msg.resume_run().run_id());
            break;
        case ServerMessage::kStopRun:
            orchestrator_->on_stop(msg.stop_run().run_id());
            log_collector_->set_run_id("_system");
            break;
        case ServerMessage::kBatchAck:
            orchestrator_->on_batch_ack(
                msg.batch_ack().run_id(), msg.batch_ack().batch_seq());
            break;
        case ServerMessage::kLogBatchAck:
            spool_->ack_log(msg.log_batch_ack().run_id(),
                             msg.log_batch_ack().batch_seq());
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// send_hello  (T037 board registration / T048 reconnect)
// ---------------------------------------------------------------------------

void App::send_hello() {
    ClientMessage msg;
    auto* hello = msg.mutable_hello();
    hello->set_board_id(cfg_.client_id);
    hello->set_client_version(ZION_CLIENT_VERSION);

    // Attach active run states from spool
    for (const auto& rs : spool_->all_run_states()) {
        auto* ai = hello->add_runs();
        ai->set_run_id(rs.run_id);
        ai->set_state(static_cast<acadia::stream::RunState>(rs.state));
        ai->set_last_batch_seq(rs.last_seq);
    }

    stream_->send(std::move(msg));
    spdlog::info("app: hello sent with {} active runs",
                 spool_->all_run_states().size());
}

// ---------------------------------------------------------------------------
// send_unacked_replay  (T048 reconnect replay)
// ---------------------------------------------------------------------------

void App::send_unacked_replay() {
    auto batches = spool_->fetch_unacked(128);
    if (batches.empty()) return;
    spdlog::info("app: replaying {} unacked batches", batches.size());
    for (const auto& row : batches) {
        TestResultBatch batch;
        if (!batch.ParseFromArray(row.payload.data(),
                                   static_cast<int>(row.payload.size()))) {
            spdlog::warn("app: replay: failed to parse batch run_id={} seq={}",
                         row.run_id, row.batch_seq);
            continue;
        }
        ClientMessage msg;
        *msg.mutable_result_batch() = std::move(batch);
        stream_.get()->send(std::move(msg));
    }
}

} // namespace zion::client

