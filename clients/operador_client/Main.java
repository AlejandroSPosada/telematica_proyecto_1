import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.Timer;
import javax.swing.UIManager;
import javax.swing.table.DefaultTableModel;

public class Main {
    public static void main(String[] args) {
        AppLogger.info("operator_client iniciado");
        SwingUtilities.invokeLater(() -> {
            try {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } catch (Exception ignored) {
            }
            OperatorApp app = new OperatorApp();
            app.setVisible(true);
            app.refreshDataAsync(true);
        });
    }
}

class OperatorApp extends JFrame {
    private final JTextField hostField = new JTextField("localhost", 12);
    private final JTextField portField = new JTextField("5000", 5);
    private final JTextField userField = new JTextField("admin", 10);
    private final JPasswordField passField = new JPasswordField("admin123", 10);
    private final JComboBox<String> filterBox = new JComboBox<>(new String[] {
        "all", "temperature", "pressure", "vibration", "humidity", "energy"
    });

    private final DefaultTableModel sensorsModel = new DefaultTableModel(
        new String[] {"ID", "Tipo", "Ultimo valor"}, 0
    ) {
        @Override
        public boolean isCellEditable(int row, int column) {
            return false;
        }
    };

    private final DefaultTableModel alertsModel = new DefaultTableModel(
        new String[] {"Sensor", "Tipo", "Mensaje", "Timestamp"}, 0
    ) {
        @Override
        public boolean isCellEditable(int row, int column) {
            return false;
        }
    };

    private final DefaultTableModel historyModel = new DefaultTableModel(
        new String[] {"Timestamp", "Valor"}, 0
    ) {
        @Override
        public boolean isCellEditable(int row, int column) {
            return false;
        }
    };

    private final JTable sensorsTable = new JTable(sensorsModel);
    private final JTable alertsTable = new JTable(alertsModel);
    private final JTable historyTable = new JTable(historyModel);

    private final OperatorProtocolClient protocolClient = new OperatorProtocolClient(this::onPushAlert, this::onConnectionLost);
    private final Timer refreshTimer;
    private volatile boolean authenticated;
    private final AtomicBoolean refreshInProgress = new AtomicBoolean(false);

    OperatorApp() {
        super("Operador IoT - Dashboard Desktop");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(1100, 700);
        setLocationRelativeTo(null);

        buildUi();

        refreshTimer = new Timer(5000, e -> refreshDataAsync(false));
        refreshTimer.setRepeats(true);
    }

    private void buildUi() {
        JPanel root = new JPanel(new BorderLayout(8, 8));
        root.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        setContentPane(root);

        JPanel top = new JPanel(new BorderLayout(8, 8));
        top.add(buildConnectionPanel(), BorderLayout.NORTH);
        top.add(buildActionsPanel(), BorderLayout.SOUTH);
        root.add(top, BorderLayout.NORTH);

        JScrollPane sensorsScroll = new JScrollPane(sensorsTable);
        sensorsScroll.setBorder(BorderFactory.createTitledBorder("Sensores"));

        JScrollPane alertsScroll = new JScrollPane(alertsTable);
        alertsScroll.setBorder(BorderFactory.createTitledBorder("Alertas"));

        JScrollPane historyScroll = new JScrollPane(historyTable);
        historyScroll.setBorder(BorderFactory.createTitledBorder("Historial del sensor seleccionado"));

        JSplitPane rightSplit = new JSplitPane(JSplitPane.VERTICAL_SPLIT, alertsScroll, historyScroll);
        rightSplit.setResizeWeight(0.6);

        JSplitPane mainSplit = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, sensorsScroll, rightSplit);
        mainSplit.setResizeWeight(0.55);
        root.add(mainSplit, BorderLayout.CENTER);

        sensorsTable.setPreferredScrollableViewportSize(new Dimension(500, 500));
        alertsTable.setPreferredScrollableViewportSize(new Dimension(450, 300));
        historyTable.setPreferredScrollableViewportSize(new Dimension(450, 200));
    }

    private JPanel buildConnectionPanel() {
        JPanel panel = new JPanel(new GridLayout(2, 4, 8, 8));
        panel.setBorder(BorderFactory.createTitledBorder("Conexion Protocolo TCP"));

        panel.add(new JLabel("Host"));
        panel.add(new JLabel("Puerto"));
        panel.add(new JLabel("Usuario"));
        panel.add(new JLabel("Password"));

        panel.add(hostField);
        panel.add(portField);
        panel.add(userField);
        panel.add(passField);

        return panel;
    }

    private JPanel buildActionsPanel() {
        JPanel panel = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 0));

        JButton connectBtn = new JButton("Conectar / Autenticar");
        JButton manualRefreshBtn = new JButton("Actualizar ahora");
        JButton historyBtn = new JButton("Cargar historial");

        connectBtn.addActionListener(e -> refreshDataAsync(true));
        manualRefreshBtn.addActionListener(e -> refreshDataAsync(false));
        historyBtn.addActionListener(e -> loadHistoryForSelectedSensor());

        filterBox.addActionListener(e -> refreshDataAsync(false));

        panel.add(connectBtn);
        panel.add(manualRefreshBtn);
        panel.add(new JLabel("Filtro tipo"));
        panel.add(filterBox);
        panel.add(historyBtn);

        return panel;
    }

    void refreshDataAsync(boolean showSuccessMessage) {
        String host = hostField.getText().trim();
        Integer port = parsePort();
        String user = userField.getText().trim();
        String pass = new String(passField.getPassword());
        String filterType = (String) filterBox.getSelectedItem();

        if (host.isEmpty() || port == null) {
            showError("Host/puerto invalido");
            return;
        }

        if ("all".equals(filterType)) {
            filterType = "";
        }

        final String finalFilterType = filterType;
        final String finalHost = host;
        final int finalPort = port;
        final String finalUser = user;
        final String finalPass = pass;

        new Thread(() -> refreshDataWorker(showSuccessMessage, finalHost, finalPort, finalUser, finalPass, finalFilterType),
            "operator-refresh").start();
    }

    private void refreshDataWorker(boolean showSuccessMessage, String host, int port, String user, String pass, String filterType) {
        if (!refreshInProgress.compareAndSet(false, true)) {
            AppLogger.info("Refresh omitido: ya hay uno en progreso");
            return;
        }

        try {
            if (!authenticated || !protocolClient.isConnected()) {
                AppLogger.info("Intentando conexion y autenticacion a " + host + ":" + port + " con usuario=" + user);
                protocolClient.connectAndAuth(host, port, user, pass);
                authenticated = true;
                AppLogger.info("Conexion y autenticacion exitosas");
            }

            List<SensorRow> sensors = protocolClient.fetchSensors(filterType);
            AppLogger.info("Datos actualizados: sensores=" + sensors.size() + ", filtro=" + (filterType == null || filterType.isEmpty() ? "all" : filterType));

            SwingUtilities.invokeLater(() -> {
                updateSensorsTable(sensors);

                if (!refreshTimer.isRunning()) {
                    refreshTimer.start();
                }

                if (showSuccessMessage) {
                    JOptionPane.showMessageDialog(this, "Conexion y autenticacion exitosas con " + host + ":" + port, "OK", JOptionPane.INFORMATION_MESSAGE);
                }
            });
        } catch (UnauthorizedException ex) {
            SwingUtilities.invokeLater(() -> {
                refreshTimer.stop();
                authenticated = false;
                AppLogger.error("Autenticacion rechazada", ex);
                showError("No autorizado (401). Verifica usuario/password.");
            });
        } catch (Exception ex) {
            SwingUtilities.invokeLater(() -> {
                refreshTimer.stop();
                authenticated = false;
                AppLogger.error("Fallo refrescando datos", ex);
                showError("Error consultando servidor: " + ex.getMessage());
            });
        } finally {
            refreshInProgress.set(false);
        }
    }

    private void loadHistoryForSelectedSensor() {
        int row = sensorsTable.getSelectedRow();
        if (row < 0) {
            showError("Selecciona un sensor en la tabla.");
            return;
        }

        String sensorId = String.valueOf(sensorsModel.getValueAt(row, 0));
        String user = userField.getText().trim();
        String pass = new String(passField.getPassword());
        String host = hostField.getText().trim();
        Integer port = parsePort();

        if (host.isEmpty() || port == null) {
            showError("Host/puerto invalido");
            return;
        }

        try {
            if (!authenticated || !protocolClient.isConnected()) {
                AppLogger.info("Reconectando para consultar historial sensor_id=" + sensorId);
                protocolClient.connectAndAuth(host, port, user, pass);
                authenticated = true;
            }

            List<HistoryRow> readings = protocolClient.fetchHistory(sensorId);
            AppLogger.info("Historial cargado sensor_id=" + sensorId + ", lecturas=" + readings.size());
            updateHistoryTable(readings);
        } catch (UnauthorizedException ex) {
            refreshTimer.stop();
            authenticated = false;
            AppLogger.error("Autenticacion rechazada al consultar historial", ex);
            showError("No autorizado (401). Verifica usuario/password.");
        } catch (Exception ex) {
            AppLogger.error("Fallo cargando historial sensor_id=" + sensorId, ex);
            showError("No se pudo cargar historial: " + ex.getMessage());
        }
    }

    private void updateSensorsTable(List<SensorRow> sensors) {
        sensorsModel.setRowCount(0);
        for (SensorRow s : sensors) {
            sensorsModel.addRow(new Object[] {
                s.id,
                s.type,
                s.lastValue
            });
        }
    }

    private void updateHistoryTable(List<HistoryRow> readings) {
        historyModel.setRowCount(0);
        for (HistoryRow r : readings) {
            historyModel.addRow(new Object[] {
                r.timestamp,
                r.value
            });
        }
    }

    private Integer parsePort() {
        String port = portField.getText().trim();

        if (port.isEmpty()) {
            return null;
        }

        try {
            int p = Integer.parseInt(port);
            if (p < 1 || p > 65535) {
                return null;
            }
            return p;
        } catch (NumberFormatException ex) {
            return null;
        }
    }

    private void showError(String msg) {
        AppLogger.error("UI error: " + msg);
        JOptionPane.showMessageDialog(this, msg, "Error", JOptionPane.ERROR_MESSAGE);
    }

    private void onPushAlert(AlertRow alert) {
        AppLogger.info("Alerta push recibida sensor=" + alert.sensorId + " tipo=" + alert.type + " timestamp=" + alert.timestamp + " mensaje=" + alert.message);
        SwingUtilities.invokeLater(() -> {
            alertsModel.insertRow(0, new Object[] {
                alert.sensorId,
                alert.type,
                alert.message,
                alert.timestamp
            });
        });
    }

    private void onConnectionLost(String reason) {
        SwingUtilities.invokeLater(() -> {
            authenticated = false;
            refreshTimer.stop();
            AppLogger.error("Conexion perdida: " + reason);
            showError("Conexion cerrada: " + reason);
        });
    }
}

class UnauthorizedException extends Exception {
    private static final long serialVersionUID = 1L;
}

class OperatorProtocolClient {
    private static final int RESPONSE_TIMEOUT_SECONDS = 6;

    private final AlertListener alertListener;
    private final DisconnectListener disconnectListener;
    private final LinkedBlockingQueue<String> responseQueue = new LinkedBlockingQueue<>();

    private Socket socket;
    private BufferedReader in;
    private BufferedWriter out;
    private Thread readerThread;
    private volatile boolean running;

    OperatorProtocolClient(AlertListener alertListener, DisconnectListener disconnectListener) {
        this.alertListener = alertListener;
        this.disconnectListener = disconnectListener;
    }

    synchronized boolean isConnected() {
        return socket != null && socket.isConnected() && !socket.isClosed();
    }

    synchronized void connectAndAuth(String host, int port, String username, String password) throws Exception {
        close();

        AppLogger.info("Abriendo socket TCP a " + host + ":" + port);
        socket = new Socket(host, port);
        socket.setSoTimeout(0);
        in = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8));
        out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8));
        responseQueue.clear();
        running = true;

        readerThread = new Thread(this::readerLoop, "operator-protocol-reader");
        readerThread.setDaemon(true);
        readerThread.start();

        String authResp = sendAndWait("AUTH " + username + " " + password);
        ProtocolResponse parsed = ProtocolParsers.parseResponse(authResp);
        if (parsed.code == 401 || parsed.code == 403) {
            close();
            throw new UnauthorizedException();
        }
        if (parsed.code != 200) {
            close();
            throw new RuntimeException("AUTH fallido: " + authResp);
        }
    }

    List<SensorRow> fetchSensors(String filterType) throws Exception {
        String cmd = filterType == null || filterType.isEmpty() ? "LIST" : "LIST " + filterType;
        ProtocolResponse resp = ProtocolParsers.parseResponse(sendAndWait(cmd));
        if (resp.code != 200) {
            throw new RuntimeException("LIST fallido: " + resp.raw);
        }
        return ProtocolParsers.parseListResponse(resp);
    }

    List<AlertRow> fetchAlerts() throws Exception {
        ProtocolResponse resp = ProtocolParsers.parseResponse(sendAndWait("ALERTS"));
        if (resp.code != 200) {
            throw new RuntimeException("ALERTS fallido: " + resp.raw);
        }
        return ProtocolParsers.parseAlertsResponse(resp);
    }

    List<HistoryRow> fetchHistory(String sensorId) throws Exception {
        ProtocolResponse resp = ProtocolParsers.parseResponse(sendAndWait("HISTORY " + sensorId));
        if (resp.code == 404) {
            return Collections.emptyList();
        }
        if (resp.code != 200) {
            throw new RuntimeException("HISTORY fallido: " + resp.raw);
        }
        return ProtocolParsers.parseHistoryResponse(resp);
    }

    synchronized void close() {
        running = false;
        try {
            if (socket != null && !socket.isClosed()) {
                AppLogger.info("Cerrando socket TCP");
                socket.close();
            }
        } catch (IOException ignored) {
            AppLogger.error("Error cerrando socket", ignored);
        }

        socket = null;
        in = null;
        out = null;
    }

    private void readerLoop() {
        try {
            String line;
            while (running && in != null && (line = in.readLine()) != null) {
                AppLogger.info("RX " + line);
                if (line.startsWith("ALERT ")) {
                    AlertRow alert = ProtocolParsers.parsePushAlert(line);
                    if (alert != null) {
                        alertListener.onAlert(alert);
                    }
                } else if (line.startsWith("RESPONSE ")) {
                    responseQueue.offer(line);
                }
            }
        } catch (IOException ex) {
            if (running) {
                AppLogger.error("Error en readerLoop", ex);
                disconnectListener.onDisconnect(ex.getMessage());
            }
        } finally {
            if (running) {
                disconnectListener.onDisconnect("servidor cerro la conexion");
            }
            close();
        }
    }

    private String sendAndWait(String command) throws Exception {
        synchronized (this) {
            if (!isConnected()) {
                throw new RuntimeException("No hay conexion activa");
            }
            AppLogger.info("TX " + maskCommand(command));
            out.write(command);
            out.write("\n");
            out.flush();
        }

        String response = responseQueue.poll(RESPONSE_TIMEOUT_SECONDS, TimeUnit.SECONDS);
        if (response == null) {
            throw new RuntimeException("Timeout esperando respuesta para: " + command);
        }
        return response;
    }

    private String maskCommand(String command) {
        if (command == null) {
            return "";
        }
        if (command.startsWith("AUTH ")) {
            String[] p = command.split("\\s+");
            if (p.length >= 3) {
                return "AUTH " + p[1] + " ****";
            }
            return "AUTH ****";
        }
        return command;
    }
}

class AppLogger {
    private static final Path LOG_PATH = Paths.get("operator_client.log");
    private static final DateTimeFormatter TS_FMT = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
    private static final long MAX_LOG_BYTES = 2L * 1024L * 1024L;

    static synchronized void info(String message) {
        write("INFO", message, null);
    }

    static synchronized void error(String message) {
        write("ERROR", message, null);
    }

    static synchronized void error(String message, Throwable t) {
        write("ERROR", message, t);
    }

    private static void write(String level, String message, Throwable t) {
        try {
            rotateIfNeeded();

            StringBuilder line = new StringBuilder();
            line.append(LocalDateTime.now().format(TS_FMT))
                .append(" [")
                .append(level)
                .append("] ")
                .append(message == null ? "" : message)
                .append(System.lineSeparator());

            if (t != null) {
                line.append(LocalDateTime.now().format(TS_FMT))
                    .append(" [")
                    .append(level)
                    .append("] ")
                    .append(t.getClass().getSimpleName())
                    .append(": ")
                    .append(t.getMessage())
                    .append(System.lineSeparator());
            }

            Files.write(
                LOG_PATH,
                line.toString().getBytes(StandardCharsets.UTF_8),
                StandardOpenOption.CREATE,
                StandardOpenOption.APPEND
            );
        } catch (IOException ignored) {
        }
    }

    private static void rotateIfNeeded() throws IOException {
        if (!Files.exists(LOG_PATH)) {
            return;
        }
        long size = Files.size(LOG_PATH);
        if (size < MAX_LOG_BYTES) {
            return;
        }

        Path backup = Paths.get("operator_client.log.1");
        Files.deleteIfExists(backup);
        Files.move(LOG_PATH, backup);
    }
}

interface AlertListener {
    void onAlert(AlertRow alert);
}

interface DisconnectListener {
    void onDisconnect(String reason);
}

class SensorRow {
    final int id;
    final String type;
    final String lastValue;

    SensorRow(int id, String type, String lastValue) {
        this.id = id;
        this.type = type;
        this.lastValue = lastValue;
    }
}

class AlertRow {
    final int sensorId;
    final String type;
    final String message;
    final String timestamp;

    AlertRow(int sensorId, String type, String message, String timestamp) {
        this.sensorId = sensorId;
        this.type = type;
        this.message = message;
        this.timestamp = timestamp;
    }
}

class HistoryRow {
    final String timestamp;
    final String value;

    HistoryRow(String timestamp, String value) {
        this.timestamp = timestamp;
        this.value = value;
    }
}

class ProtocolResponse {
    final String raw;
    final int code;
    final List<String> payload;

    ProtocolResponse(String raw, int code, List<String> payload) {
        this.raw = raw;
        this.code = code;
        this.payload = payload;
    }
}

class ProtocolParsers {
    static ProtocolResponse parseResponse(String line) {
        String[] parts = line.trim().split("\\s+");
        if (parts.length < 3 || !"RESPONSE".equals(parts[0])) {
            throw new IllegalArgumentException("Respuesta invalida: " + line);
        }

        int code = Integer.parseInt(parts[1]);
        List<String> payload = new ArrayList<>();
        for (int i = 2; i < parts.length; i++) {
            payload.add(parts[i]);
        }
        return new ProtocolResponse(line, code, payload);
    }

    static List<SensorRow> parseListResponse(ProtocolResponse resp) {
        if (resp.payload.isEmpty()) {
            return Collections.emptyList();
        }

        int count = Integer.parseInt(resp.payload.get(0));
        List<SensorRow> out = new ArrayList<>();

        int idx = 1;
        for (int i = 0; i < count && idx + 2 < resp.payload.size(); i++) {
            int id = Integer.parseInt(resp.payload.get(idx++));
            String type = resp.payload.get(idx++);
            String lastValue = resp.payload.get(idx++);
            out.add(new SensorRow(id, type, lastValue));
        }
        return out;
    }

    static List<AlertRow> parseAlertsResponse(ProtocolResponse resp) {
        if (resp.payload.isEmpty()) {
            return Collections.emptyList();
        }

        int count = Integer.parseInt(resp.payload.get(0));
        List<AlertRow> out = new ArrayList<>();

        int idx = 1;
        for (int i = 0; i < count && idx + 2 < resp.payload.size(); i++) {
            int sensorId = Integer.parseInt(resp.payload.get(idx++));
            String type = resp.payload.get(idx++);
            String timestamp = resp.payload.get(idx++);
            out.add(new AlertRow(sensorId, type, "-", timestamp));
        }

        Collections.reverse(out);
        return out;
    }

    static List<HistoryRow> parseHistoryResponse(ProtocolResponse resp) {
        if (resp.payload.isEmpty()) {
            return Collections.emptyList();
        }

        int count = Integer.parseInt(resp.payload.get(0));
        List<HistoryRow> out = new ArrayList<>();

        int idx = 1;
        for (int i = 0; i < count && idx + 1 < resp.payload.size(); i++) {
            String timestamp = resp.payload.get(idx++);
            String value = resp.payload.get(idx++);
            out.add(new HistoryRow(timestamp, value));
        }
        return out;
    }

    static AlertRow parsePushAlert(String line) {
        String[] parts = line.trim().split("\\s+");
        if (parts.length < 5 || !"ALERT".equals(parts[0])) {
            return null;
        }

        int sensorId;
        try {
            sensorId = Integer.parseInt(parts[1]);
        } catch (NumberFormatException ex) {
            return null;
        }

        String type = parts[2];
        String timestamp = parts[3];
        StringBuilder msg = new StringBuilder();
        for (int i = 4; i < parts.length; i++) {
            if (i > 4) msg.append(' ');
            msg.append(parts[i]);
        }

        return new AlertRow(sensorId, type, msg.toString(), timestamp);
    }
}
