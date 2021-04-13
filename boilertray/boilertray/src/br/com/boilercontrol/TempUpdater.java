package br.com.boilercontrol;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.TrayIcon;
import java.awt.image.BufferedImage;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class TempUpdater implements Runnable {

	private static final String AUTH_HEADER = "Basic Ym9pbGVyY29udHJvbDphYmNkMTIzNCFAIyQ=";

	private TrayIcon trayIcon;
	
	public TempUpdater(TrayIcon trayIcon) {
		this.trayIcon = trayIcon;
	}
	
	@Override
	public void run() {
		for (;;) {
			try {
				Thread.sleep(5000);

				String data = getTemp();
				String temp = getTempFromJson(data);

				Boolean pumpOn = data.indexOf("\"pumpStatus\":\"enabled\"") > 0;
				Boolean heaterOn = data.indexOf("\"heaterStatus\":\"enabled\"") > 0;
				
				if (heaterOn) {
					trayIcon.setImage(createImage(temp, Colors.HEATER_ON));
				} else if (pumpOn) {
					trayIcon.setImage(createImage(temp, Colors.PUMP_ON));
				} else {
					trayIcon.setImage(createImage(temp, Colors.ALL_OFF));
				}
				trayIcon.setToolTip("Boiler temp at " + temp + "°");

			} catch (Exception e) {
				// TODO: handle exception
			} 
		}
	}

	// Gera o icone com a temperatura
	protected static Image createImage(String temp, Colors color) {
		BufferedImage bufferedImage = new BufferedImage(16, 16, BufferedImage.TYPE_INT_RGB);
		Graphics graphics = bufferedImage.createGraphics();
		graphics.setColor(new Color(10,32,47));
		graphics.fillRect(0, 0, 16, 16);
		graphics.setColor(color.cor);
		graphics.setFont(new Font("Tahoma", Font.PLAIN, 10));
		graphics.drawString(String.valueOf(Double.valueOf(temp.trim()).intValue()) + "°", 2, 12);

		return bufferedImage;
	}
	
	// Get current temp;
	protected static String getTemp() {
		try {
			URL url = new URL("https://italopulga.ddns.net/status");
			HttpURLConnection con = (HttpURLConnection) url.openConnection();
			con.setRequestMethod("GET");
			con.setRequestProperty("Authorization", AUTH_HEADER);
			con.setDoOutput(true);
			con.setReadTimeout(5000);
			con.setConnectTimeout(5000);


			// To store our response
			StringBuilder content;

			// Get the input stream of the connection
			try (BufferedReader input = new BufferedReader(new InputStreamReader(con.getInputStream()))) {
			    String line;
			    content = new StringBuilder();
			    while ((line = input.readLine()) != null) {
			        // Append each line of the response and separate them
			        content.append(line);
			        content.append(System.lineSeparator());
			    }
			} finally {
			    con.disconnect();
			}

			return content.toString();
		} catch (Exception e) {
			return "00";
		}
	}
	
	// TODO: Fazer um parse usando uma biblioteca
	private static String getTempFromJson(String data) {
		String partial = data.substring(data.indexOf("reservoirTemp\":\"") + 16);
		return partial.substring(0, partial.indexOf("."));
	}
}
