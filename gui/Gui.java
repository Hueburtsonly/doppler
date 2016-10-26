import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.Vector;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class Gui extends JPanel {

	double phase = 0; // degrees
	double power = 0; // dBm

	public static void main(String[] args) throws IOException {
		JFrame frame = new JFrame("Doppler");
		Gui gui = new Gui();
		frame.add(gui);
		frame.setBounds(50, 350, 800, 500);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setVisible(true);

		gui.run();
	}

	public Gui() {}

	public void paintComponent(Graphics g) {
		super.paintComponent(g);

		g.drawString("Phase = " + phase, 20,20);
		g.drawString("Power = " + power, 20,40);
	}


	public void run() throws IOException {
		BufferedReader bufferReader = new BufferedReader(new InputStreamReader(System.in));
		while (true) {
			String[] parts = bufferReader.readLine().split(",");
			phase = Double.parseDouble(parts[0]);
			power = Double.parseDouble(parts[1]);
			repaint();
		}
	}




}