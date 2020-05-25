package br.com.boilercontrol;

import java.awt.AWTException;
import java.awt.Desktop;
import java.awt.MenuItem;
import java.awt.PopupMenu;
import java.awt.SystemTray;
import java.awt.TrayIcon;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;
import java.net.URI;

import javax.swing.SwingUtilities;
import javax.swing.UIManager;

public class BoilerTray {
	
	public static void main(String[] args) {
		/* Use an appropriate Look and Feel */
		try {
			UIManager.setLookAndFeel("com.sun.java.swing.plaf.windows.WindowsLookAndFeel");
		} catch (Exception e) {
			e.printStackTrace();
		}

		/* Turn off metal's use of bold fonts */
		UIManager.put("swing.boldMetal", Boolean.FALSE);
		// Schedule a job for the event-dispatching thread:
		// adding TrayIcon.
		SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				createAndShowGUI();
			}
		});
	}

	private static void createAndShowGUI() {
		final PopupMenu popup = new PopupMenu();
		final SystemTray tray = SystemTray.getSystemTray();

		// Create a popup menu components
		MenuItem exitItem = new MenuItem("Exit");

		// Add components to popup menu
		popup.add(exitItem);

		TrayIcon trayIcon = new TrayIcon(new BufferedImage(16,16,1));
		trayIcon.setPopupMenu(popup);
		trayIcon.setImageAutoSize(Boolean.TRUE);

		try {
			tray.add(trayIcon);
		} catch (AWTException e) {
			System.out.println("TrayIcon could not be added.");
			return;
		}

		trayIcon.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				if (Desktop.isDesktopSupported() && Desktop.getDesktop().isSupported(Desktop.Action.BROWSE)) {
					try {
						Desktop.getDesktop().browse(new URI("https://italopulga.ddns.net"));
					} catch (Exception ex) {
						// TODO: handle exception
					}
				}
			}
		});

		exitItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				tray.remove(trayIcon);
				System.exit(0);
			}
		});
		
		// Thread that will update temp.
		Thread t1 = new Thread(new TempUpdater(trayIcon));
		t1.start();
	}
}
