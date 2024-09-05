import java.io.*;
import java.net.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public class MyWebServer {
    //Create responses with desired specs
    public static void sendResponse(int statusCode, String statusMessage, Date lastModified, long contentLength, OutputStream out) throws IOException {
        SimpleDateFormat sdf = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z", Locale.US);
        sdf.setTimeZone(TimeZone.getTimeZone("EST"));
        String date = sdf.format(new Date());

        String response = "HTTP/1.1 " + statusCode + " " + statusMessage + "\r\n" +
                "Date: " + date + "\r\n" +
                "Server: MyWebServer\r\n";

        if (lastModified != null) {
            response += "Last-Modified: " + sdf.format(lastModified) + "\r\n";
        }

        response += "Content-Length: " + contentLength + "\r\n" +
                "\r\n";

        out.write(response.getBytes());
    }
    public static void main(String[] args) throws IOException {
        //Make Sure We have Command Line args
        if (args.length != 2) {
            System.err.println("Usage: java MyWebServer <port> <root_directory>");
            System.exit(1);
        }
        //Parse command line
        int port = Integer.parseInt(args[0]);
        String rootDir = args[1];
        //Create Socket on designated port
        ServerSocket serverSocket = new ServerSocket(port);
        System.out.println("Server listening on port " + port + ", root directory: " + rootDir);
        //Open Socket and loop to run web server
        while (true) {
            Socket clientSocket = serverSocket.accept();
            try (BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
                 OutputStream out = clientSocket.getOutputStream()) {

                String requestLine = in.readLine();
                // Ignore empty requests
                if (requestLine == null) return;

                String[] request = requestLine.split(" ");
                if (request.length < 2) return;
                String method = request[0];
                String requestFile = request[1];

                if (!method.equals("GET") && !method.equals("HEAD")) {
                    sendResponse(501, "Not Implemented", null, 0, out);
                    return;
                }
                //Serve index.html
                if (requestFile.startsWith("/")) {
                    requestFile = requestFile.substring(1);
                }

                // AbsoluteURI support
                if (requestFile.startsWith("http://")) {
                    int index = requestFile.indexOf("/", 7);
                    if (index != -1) {
                        requestFile = requestFile.substring(index);
                    } else {
                        sendResponse(400, "Bad Request", null, 0, out);
                        return;
                    }
                }
                //Check for file path and directory
                String filePath = rootDir + File.separator + requestFile;
                File file = new File(filePath);

                if (file.isDirectory()) {
                    // If directory, try to serve index.html
                    File indexFile = new File(filePath + File.separator + "index.html");
                    if (indexFile.exists() && indexFile.isFile()) {
                        file = indexFile;
                    } else {
                        sendResponse(404, "Not Found", null, 0, out);
                        return;
                    }
                }
                //File not found error code 404
                if (!file.exists() || file.isDirectory()) {
                    sendResponse(404, "Not Found", null, 0, out);
                    return;
                }
                //Date formatting
                SimpleDateFormat sdf = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z", Locale.US);
                sdf.setTimeZone(TimeZone.getTimeZone("GMT"));
                Date lastModified = new Date(file.lastModified());

                // Check If-Modified-Since header
                String ifModifiedSinceHeader = null;
                while (true) {
                    String line = in.readLine();
                    if (line == null || line.isEmpty()) {
                        break;
                    } else if (line.startsWith("If-Modified-Since:")) {
                        ifModifiedSinceHeader = line.substring("If-Modified-Since:".length()).trim();
                    }
                }

                if (ifModifiedSinceHeader != null) {
                    Date ifModifiedSinceDate = sdf.parse(ifModifiedSinceHeader);
                    if (ifModifiedSinceDate.getTime() >= lastModified.getTime()) {
                        sendResponse(304, "Not Modified", lastModified, 0, out);
                        return;
                    }
                }
                //No issues, send OK
                sendResponse(200, "OK", lastModified, file.length(), out);
                //Get and serve file
                if (method.equals("GET")) {
                    try (FileInputStream fileInputStream = new FileInputStream(file)) {
                        byte[] buffer = new byte[1024];
                        int bytesRead;
                        while ((bytesRead = fileInputStream.read(buffer)) != -1) {
                            out.write(buffer, 0, bytesRead);
                        }
                    }
                }
            //Exception catching
            } catch (Exception e) {
                e.printStackTrace();
            }
            //Close socket connection
            finally {
                try {
                    clientSocket.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
