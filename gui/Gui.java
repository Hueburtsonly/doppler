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

	double freq = 2.943e9;
	final static double c = 299792458000.0;

	public static void main(String[] args) throws Exception {
		JFrame frame = new JFrame("Doppler");
		Gui gui = new Gui();
		frame.add(gui);
		frame.setBounds(50, 350, 800, 500);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setVisible(true);

		gui.run(frame);
	}

	public Gui() {
		//setOpaque(true);
		setBackground(Color.BLACK);
		setForeground(Color.GREEN);
	}

	public void paintComponent(Graphics g1) {
		super.paintComponent(g1);
		Graphics2D g = (Graphics2D)g1;
		g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
		g.setStroke(new BasicStroke(3));
		g.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 24));
		int width = getWidth();
		int height = getHeight();
		int cx = width/2;
		int cy = height/2;
		int r = height / 3;
		int toph = 60;
		g.drawString(String.format("Phase =%6.1f°", phase), 20,40);
		g.drawString(String.format("Power =%6.1f dBm", power), width/2, 40);
		//g.drawLine(cx, cy, cx + (int)(r * Math.cos(phase / 180 * Math.PI)), cy + (int)(r * Math.sin(phase / 180 * Math.PI)));

		g.setColor(Color.YELLOW);
		int et = (int)((height - 21-toph) * (20 - power) / 170.0);
		g.fillRect(width - 70, toph+et, 70-21, height - 21-toph-et);
		g.drawRect(width - 70, toph, 70-21, height - 21-toph);

		g.setColor(Color.WHITE);
		g.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 18));
		int by = toph + (height-21-toph)/2;
		int sy = (height-21-toph)/2;
		int wavwidth = 800;
		final int N = width - 110 - 20;
		g.drawLine(20, by, width - 110, by);
		g.drawLine(20, by-sy, 20, by+sy);
		for (boolean k = false; ; k = true) {
k=true;
			// lambda = c / f
			// mm     = mm / s  / (1/s)
			final double stride = k ? (wavwidth * 100 / (1e12 / freq)) : (wavwidth * 50 / (c / freq));
			final double val = k ? 100 : 50;
			final String format  = k ? "%.0f ps" : "%.0f mm";
			double nval = 0;
			for (double i = stride; i < N-30; i += stride) {
				nval += val;
				g.drawLine(20 + (int)i, by, 20 + (int)i, by + (k ? -20 : 20));
				g.drawString(String.format(format, nval), 20 + (int)i - 30, by + (k ? -30 : 46));
			}
			if (k) {
				break;
			}
		}

		g.setColor(new Color(255,128,0));

		int xpoints[] = new int[N];
		int ypoints[] = new int[N];
		for (int i=0; i < N; i++) {
			xpoints[i] = 20 + i;
			ypoints[i] = (int)(-Math.sin((i * Math.PI / wavwidth * 2 ) + (-phase / 180 * Math.PI)) * 0.8 * sy + by);
		}
		g.drawPolyline(xpoints, ypoints, N);

	}


    public void run(JFrame frame) throws Exception {
		BufferedReader bufferReader = new BufferedReader(new InputStreamReader(System.in));
		while (true) {
			String[] parts = bufferReader.readLine().split(",");
			phase = phase + 0.3 * ((Double.parseDouble(parts[0]) - phase+720+180)%360-180);
			power = power * 0.7 + 0.3 * Double.parseDouble(parts[1]);
			repaint();
			frame.getToolkit().sync();
			Thread.sleep(10);
		}
	}




}
