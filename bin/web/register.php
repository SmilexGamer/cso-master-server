<?php
/**
 * PHP Registration Page - Using bcrypt hashing algorithm to protect user passwords
 * 
 * This file implements a secure user registration system using PHP's native password_hash() function
 * with the PASSWORD_BCRYPT algorithm. bcrypt automatically handles salt generation and storage,
 * and provides good protection against brute force attacks.
 *
 * Repository: cso-master-server
 * Contributors: Huan912, Smilex_Gamer
 * Before use, please ensure database connection parameters are configured
 */

/*
// https://www.php.net/manual/en/timezones.php
date_default_timezone_set('Asia/Taipei'); // If you want to specify a time zone for the registration time
*/

$username = $password = $confirm_password = "";
$username_err = $password_err = $confirm_password_err = "";
$registration_success = false;

//DB Config
$servername = "localhost";
$db_username = "root";
$db_password = "";
$dbname = "csodatabase";

if ($_SERVER["REQUEST_METHOD"] == "POST") {

    if (empty(trim($_POST["username"]))) {
        $username_err = "Please enter your username";
    } elseif (strlen(trim($_POST["username"])) < 4) {
        $username_err = "Username must contain at least 4 characters";
    } elseif (strlen(trim($_POST["username"])) > 16) {
        $username_err = "Username can only contain up to 16 characters";
    } elseif (!preg_match('/^[a-zA-Z0-9]+$/', trim($_POST["username"]))) {
        $username_err = "Username can only contain letters and numbers";
    } else 
    {
        $mysqli = new mysqli($servername, $db_username, $db_password, $dbname);
        if ($mysqli->connect_error) {
            die("connect error: " . $mysqli->connect_error);
        }
        //Check if the user is registered
        $sql = "SELECT 1 FROM Users WHERE userName = ?";
        if ($stmt = $mysqli->prepare(query: $sql)) {
            $stmt->bind_param("s", $param_username);
            $param_username = trim($_POST["username"]);
            if ($stmt->execute()) {
                $stmt->store_result();
                if ($stmt->num_rows > 0) {
                    $username_err = "This username is already in use";
                } else {
                    $username = trim($_POST["username"]);
                }
            } else {
                echo "There was an error, Please try again later";
            }
            $stmt->close();
        }
        $mysqli->close();
    }
    
    // Check password
    if (empty(trim($_POST["password"]))) {
        $password_err = "Please enter your password";
    } elseif (strlen(trim($_POST["password"])) < 8) {
        $password_err = "Passwords must contain at least 8 characters";
    } else {
        $password = trim($_POST["password"]);
    }
    
    // Check confirm
    if (empty(trim($_POST["confirm_password"]))) {
        $confirm_password_err = "Please enter your password again";
    } else {
        $confirm_password = trim($_POST["confirm_password"]);
        if (empty($password_err) && ($password != $confirm_password)) {
            $confirm_password_err = "Password mismatch";
        }
    }
    
    if (empty($username_err) && empty($password_err) && empty($confirm_password_err)) {
        $mysqli = new mysqli($servername, $db_username, $db_password, $dbname);
        if ($mysqli->connect_error) {
            die("connect error: " . $mysqli->connect_error);
        }
        // Add account to sql table
        $sql = "INSERT INTO Users (userName, password, registerTime) VALUES (?, ?, ?)";
        if ($stmt = $mysqli->prepare($sql)) {

            $stmt->bind_param("sss", $param_username, $param_password, $param_registerTime);
            $param_username = $username;
            $param_password = password_hash($password, PASSWORD_BCRYPT);
            $param_registerTime = date('Y-m-d H:i:s');
            if ($stmt->execute()) {
                $registration_success = true;
            } else {
                echo "There was an error, Please try again later";
            }
            $stmt->close();
        }
        $mysqli->close();
    }
}
?>

<!DOCTYPE html>
<html lang="zh-EN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Register</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            margin: 0;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
        }
        .wrapper {
            width: 400px;
            padding: 20px;
            background: white;
            border-radius: 5px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }
        h2 {
            text-align: center;
            color: #333;
            margin-bottom: 20px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
        }
        input[type="text"], input[type="email"], input[type="password"] {
            width: 100%;
            padding: 10px;
            box-sizing: border-box;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 16px;
        }
        .alert {
            color: #dc3545;
            font-size: 14px;
            margin-top: 5px;
        }
        .success {
            color: #28a745;
            padding: 15px;
            margin-bottom: 20px;
            text-align: center;
            border: 1px solid #d4edda;
            border-radius: 4px;
            background-color: #d4edda;
        }
        .hashed-example {
            background-color: #f8f9fa;
            padding: 15px;
            margin-top: 20px;
            border-radius: 4px;
            font-family: monospace;
            word-break: break-all;
        }
        .hashed-example h3 {
            margin-top: 0;
            color: #333;
        }
        button[type="submit"] {
            width: 100%;
            background-color: #007bff;
            color: white;
            padding: 12px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
        }
        button[type="submit"]:hover {
            background-color: #0069d9;
        }
        .login-link {
            text-align: center;
            margin-top: 15px;
        }
    </style>
</head>
<body>
    <div class="wrapper">
        <?php if ($registration_success): ?>
            <div class="success">Registration was successful</div>
            <p>Your account has been successfully created.</p>
        <?php else: ?>
            <h2>Register Account</h2>
            <form action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]); ?>" method="post">
                <div class="form-group">
                    <label>Username</label>
                    <input type="text" name="username" value="<?php echo $username; ?>">
                    <span class="alert"><?php echo $username_err; ?></span>
                </div>
                <div class="form-group">
                    <label>Password</label>
                    <input type="password" name="password">
                    <span class="alert"><?php echo $password_err; ?></span>
                </div>
                <div class="form-group">
                    <label>Confirm Password</label>
                    <input type="password" name="confirm_password">
                    <span class="alert"><?php echo $confirm_password_err; ?></span>
                </div>
                <div class="form-group">
                    <button type="submit">Submit</button>
                </div>
            </form>
        <?php endif; ?>
    </div>
</body>
</html>